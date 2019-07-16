#include "gpuWorker.hpp"
#include <assert.h>
#include <nanomsg/nn.h>
#include <nanomsg/reqrep.h>
#include <nanomsg/pair.h>
#include <nanomsg/tcp.h>
#include "protobuf/args.pb.h"
#include "tools.hpp"
#include <iostream>
#include <chrono>
using namespace std::chrono_literals;

gpuWorker::gpuWorker(mc* _pdamc,int _streamNum, std::vector<float>* _d,int _fretHistNum,
        std::mutex *m, std::condition_variable *cv,int *_dataready,int *_sn,
  std::vector<float> *_params, int *_ga_start, int *_ga_stop,int *_N){
    pdamc=_pdamc;
    _m=m;
    _cv=cv;
    streamNum=_streamNum;
    pdamc->set_gpuid();       
    SgDivSr=_d;
    fretHistNum=_fretHistNum;
    dataready=_dataready;
    s_n=_sn;
    params=_params;
    ga_start=_ga_start;
    ga_stop=_ga_stop;
    N=_N;
}
// template <typename Tag, typename Storage>
// auto gpuWorker::mkhist(std::vector<float>* SgDivSr,int binnum,float lv,float uv){
//     auto h = make_s(static_tag(), std::vector<float>(), reg(binnum, lv, uv));
//     for (auto it = SgDivSr->begin(), end = SgDivSr->end(); it != end;) 
//         h(*it++);
//     // auto h = make_histogram(
//     //   axis::regular<>(binnum, 0.0, 1.0, "x")
//     // );    
//     // std::for_each(SgDivSr->begin(), SgDivSr->end(), std::ref(h));
//     return h;
// }
void gpuWorker::run(int sz_burst){
    // auto fretHist=mkhist(SgDivSr,fretHistNum,0,1);
    int countcalc=0;
    do {            
      for(int sid=0;sid<streamNum;sid++){
        std::unique_lock<std::mutex> lck(_m[sid],std::defer_lock);
        // if(!lck.try_lock_for(500ms))
        if(!lck.try_lock()){
          // std::this_thread::sleep_for(200ms);
          continue;
        }
        if (dataready[sid]==3){
          if(pdamc->streamQuery(sid)){
            dataready[sid]=4;
            lck.unlock();
            _cv[sid].notify_one();
            continue;
          }          
        }else if(dataready[sid]==4||dataready[sid]==0){
          lck.unlock();
          continue;          
        }
        else if(!_cv[sid].wait_for(lck,500ms,[this,sid]{return (dataready[sid]==1|| 
            dataready[sid]==2);})){
          lck.unlock();
          continue;
        }
        pdamc->set_nstates(s_n[sid],sid);
        pdamc->set_params(s_n[sid],sid,params[sid]);
        N[sid]=pdamc->setBurstBd(ga_start[sid],ga_stop[sid], sid);
        pdamc->run_kernel(N[sid],sid);
        dataready[sid]=3;
        if(pdamc->streamQuery(sid)){
          dataready[sid]=4;
          lck.unlock();
          _cv[sid].notify_one();
        }
        else{
          lck.unlock();
          continue;
        }  
      }
      countcalc++;
    }while(countcalc<3);
    // std::cout<<"end\n";
}