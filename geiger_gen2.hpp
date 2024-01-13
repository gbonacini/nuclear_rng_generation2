// -----------------------------------------------------------------
// nuclear rng - generation 2
// Copyright (C) 2023  Gabriele Bonacini
//
// This program is distributed under dual license:
// - Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0) License
// for non commercial use, the license has the following terms:
// * Attribution — You must give appropriate credit, provide a link to the license,
// and indicate if changes were made. You may do so in any reasonable manner,
// but not in any way that suggests the licensor endorses you or your use.
// * NonCommercial — You must not use the material for commercial purposes.
// A copy of the license it's available to the following address:
// http://creativecommons.org/licenses/by-nc/4.0/
// - For commercial use a specific license is available contacting the author.
// -----------------------------------------------------------------

#pragma once

#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/cyw43_arch.h" 
#include "pico/mutex.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"

#include "lwip/pbuf.h"
#include "lwip/tcp.h"

#include <iostream>
#include <array>
#include <deque>
#include <utility>
#include <string>
#include <algorithm>
#include <limits>

namespace geigergen2 {

    using std::cerr,
          std::array,
          std::string,
          std::to_string,
          std::copy_n,
          std::deque,
          std::numeric_limits;

    using  rng=unsigned char;
    using  registry=unsigned int;
    static_assert(  numeric_limits<rng>::max() <  numeric_limits<registry>::max() ); 
    using  Rng=std::pair<rng, registry>;

    class GeigerGen2 {
        public:
            static inline const unsigned int                       MIN_RESULT           { 0 },
                                                                   MAX_RESULT           { 15 },
                                                                   INVALID_RESULT       { MAX_RESULT + 1 };

            static_assert( MIN_RESULT <  MAX_RESULT ); 
            static_assert( INVALID_RESULT <  numeric_limits<registry>::max() ); 
            static_assert( MAX_RESULT < INVALID_RESULT ); 

            static GeigerGen2*     getInstance(unsigned int  pin, 
                                               unsigned int  vthr,
                                               unsigned int  zero)     noexcept;
            void                   init(void)                          noexcept;
            static void            abort(const char* msg)              noexcept;
            void                   detect(void)                        noexcept;
            static Rng             getRnd(void)                        noexcept;
            static size_t          getAvailable(void)                  noexcept;

        private:
            static inline mutex_t                                  rndMutex;
            static inline deque<Rng>                               rndQueue; 
            static inline const size_t                             MAX_QUEUE_LEN        { 10240 };
            static inline long                                     count                { 0L },
                                                                   genCount             { 0L },
                                                                   lastCount            { 0L };
            static inline unsigned int                             gpioPin,
                                                                   vthreshold,
                                                                   zerothreshold;

            static inline GeigerGen2*                              instance             { nullptr };
            static inline unsigned int                             roulette             { 0 },
                                                                   lastRnd              { INVALID_RESULT };

            explicit GeigerGen2(unsigned int pin, 
                                unsigned int vhtr,
                                unsigned int zero)                 noexcept;
    };

    GeigerGen2::GeigerGen2(unsigned int pin, unsigned int  vthr, unsigned int zero)  noexcept {
        gpioPin       = pin;
        vthreshold    = vthr;
        zerothreshold = zero;
    }

    void  GeigerGen2::abort(const char* msg) noexcept{
        cerr << "Abort : " << msg << "\n";
        for(;;) sleep_ms(1000);
    }

    Rng GeigerGen2::getRnd(void) noexcept{
        Rng ret { INVALID_RESULT, 0 };
        if( ! GeigerGen2::rndQueue.empty()){
             mutex_enter_blocking(&GeigerGen2::rndMutex);
             ret = GeigerGen2::rndQueue.front();
             GeigerGen2::rndQueue.pop_front();
             mutex_exit(&GeigerGen2::rndMutex);
        }
        return ret;
    }

    size_t  GeigerGen2::getAvailable(void)  noexcept{
          return GeigerGen2::rndQueue.size();
    }

    void GeigerGen2::init(void)  noexcept {
        stdio_init_all(); 

        adc_init();
        adc_gpio_init(gpioPin);
        adc_select_input(0);

        mutex_init(&rndMutex);
    }

    GeigerGen2* GeigerGen2::getInstance(unsigned int pin, unsigned int vthr, unsigned int zero){
        if(instance == nullptr) instance = new GeigerGen2(pin, vthr, zero);
        return instance;
    }

    void GeigerGen2::detect(void)  noexcept{
        auto detectionThread = [](){ 
           for(;;){
               uint16_t result { adc_read() };
               if(result > vthreshold){ 

                  mutex_enter_blocking(&GeigerGen2::rndMutex);
                  if(GeigerGen2::rndQueue.size() > GeigerGen2::MAX_QUEUE_LEN) GeigerGen2::rndQueue.pop_front();
                  GeigerGen2::rndQueue.push_back({GeigerGen2::roulette % (MAX_RESULT + 1), GeigerGen2::roulette});
                  mutex_exit(&GeigerGen2::rndMutex);

                  for(;;){ result = adc_read();
                        if(result > zerothreshold ) sleep_us(10);
                           else  break;
                  }
                  sleep_us(100);
                  GeigerGen2::count++;
               }
               GeigerGen2::roulette++;
           }
        };

        multicore_launch_core1(detectionThread);
    }

    using Pbuf=struct pbuf;
    using TcpPcb=struct tcp_pcb;
    static const  u16_t BUF_SIZE {2048};
    using Buffer=array<uint8_t,BUF_SIZE> ;
    struct Context {
        TcpPcb    *server_pcb,
                  *client_pcb;
        Buffer    bufferSend,
                  bufferRecv;
        u16_t     toSendLen,
                  sentLen,
                  recvLen;
    };

    class GeigerGen2NetworkLayer{
        public:
            explicit GeigerGen2NetworkLayer(u16_t port=6666)                                       noexcept;
            int      service(void)                                                                 noexcept;

        private:
            u16_t                   TCP_PORT;
            static  inline Context  context;

            static inline err_t clientClose(void *ctx)                                             noexcept;
            static inline err_t serverClose(void *ctx)                                             noexcept;
            static inline err_t serverResult(void *ctx, int status)                                noexcept;
            static inline err_t clientResult(void *ctx, int status)                                noexcept;
            static inline err_t result(void *ctx, int status)                                      noexcept;
            static inline err_t serverSentClbk(void *ctx, TcpPcb *tpcb, u16_t len)                 noexcept;
            static inline err_t serverSendData(void *ctx, TcpPcb *tpcb)                            noexcept;
            static inline err_t serverRecvClbk(void *ctx, TcpPcb *tpcb, Pbuf* pb, err_t err)       noexcept;
            static inline void  serverErrClbk(void *ctx, err_t err)                                noexcept;
            static inline err_t serverAccept(void *ctx, TcpPcb *client_pcb, err_t err)             noexcept;
    };

    GeigerGen2NetworkLayer::GeigerGen2NetworkLayer(u16_t port) noexcept
         :   TCP_PORT{port}
    {
        cerr << "Connected.\n\nStarting server at " << ip4addr_ntoa(netif_ip4_addr(netif_list))  << " on port " <<  TCP_PORT << '\n';
    }

    err_t GeigerGen2NetworkLayer::serverClose(void *ctx) noexcept{
    Context *context   { static_cast<Context*>(ctx)};
    err_t err          { ERR_OK };
    cerr << "ServerClose\n";
    if(context->server_pcb){
        tcp_arg(context->server_pcb, nullptr);
        tcp_close(context->server_pcb);
        context->server_pcb = nullptr;
    }
    return err;
}

err_t GeigerGen2NetworkLayer::clientClose(void *ctx) noexcept{
    Context *context   { static_cast<Context*>(ctx)};
    err_t err          { ERR_OK };
    cerr << "ClientClose\n";
    if(context->client_pcb != nullptr){
        tcp_arg(context->client_pcb,  nullptr);
        tcp_sent(context->client_pcb, nullptr);
        tcp_recv(context->client_pcb, nullptr);
        tcp_err(context->client_pcb,  nullptr);
        err = tcp_close(context->client_pcb);
        if (err != ERR_OK) {
            cerr << "ClientClose : Error: ClientClose : " <<  err << '\n';
            tcp_abort(context->client_pcb);
            err = ERR_ABRT;
        }
        context->client_pcb = nullptr;
    }
    return err;
}

err_t GeigerGen2NetworkLayer::serverResult(void *ctx, int status) noexcept{
    cerr << "ServerResult: ";
    ( status == 0 ) ? cerr << "success\n" : cerr << "failed: " << status << '\n';
    
    return serverClose(ctx);
}

err_t GeigerGen2NetworkLayer::clientResult(void *ctx, int status) noexcept{
    cerr << "ClientResult: ";
    ( status == 0 ) ? cerr << "success\n" : cerr << "failed: " << status << '\n';
    
    return clientClose(ctx);
}

err_t GeigerGen2NetworkLayer::result(void *ctx, int status) noexcept{
    err_t ret          { ERR_OK };
    cerr << "Result\n";
    if(serverClose(ctx) != ERR_OK) ret = ERR_ABRT;
    if(clientClose(ctx) != ERR_OK) ret = ERR_ABRT;
    return ret;
}

err_t GeigerGen2NetworkLayer::serverSentClbk(void *ctx, TcpPcb *tpcb, u16_t len) noexcept{
    Context *context { static_cast<Context*>(ctx)};
    cerr << "ServerSentClbk : bytes sent: " << len << '\n';
    context->sentLen += len;

    return ERR_OK;
}

err_t GeigerGen2NetworkLayer::serverSendData(void *ctx, TcpPcb *tpcb)  noexcept{
    Context            *context { static_cast<Context*>(ctx)};

    context->sentLen = 0;
    cerr << "ServerSendData : writing " << context->toSendLen << " bytes to client\n";
    cyw43_arch_lwip_check();
    if(err_t err { tcp_write(tpcb, context->bufferSend.data(), context->toSendLen, TCP_WRITE_FLAG_COPY) }; err != ERR_OK){
        cerr << "ServerSendData : Error writing data : " <<  err << '\n';
        return clientResult(context, -1);
    }
    tcp_output(tpcb);  
    return ERR_OK;
}

err_t GeigerGen2NetworkLayer::serverRecvClbk(void *ctx, TcpPcb *tpcb, Pbuf* pb, err_t err)  noexcept{
    Context *context { static_cast<Context*>(ctx)};
    cerr << "ServerRecvClbk\n";
    if(!pb) return clientResult(context, -1);
    cyw43_arch_lwip_check();

    context->recvLen    =   pbuf_copy_partial(pb, context->bufferRecv.data(), pb->tot_len, 0);
    cerr << "ServerRecvClbk " <<  pb->tot_len << "/" <<  context->recvLen << " err " << static_cast<int>(err) << '\n';

    tcp_recved(tpcb, pb->tot_len);
    pbuf_free(pb);
    
    for(u16_t i{0}; i<context->recvLen; i=i+3){
        cerr << "ServerRecvClbk : Interation : " << ( (3 + i ) /3 )  << " of " << context->recvLen / 3
             << " payload: " <<  context->bufferRecv.at(0 + i) << " - "
             <<  context->bufferRecv.at(1 + i) << " - "
             <<  context->bufferRecv.at(2 + i) << '\n';
    
        auto ckeckReq = [&]() -> int  {   if(     context->bufferRecv.at(0 + i) == 'r' && 
                                                  context->bufferRecv.at(1 + i) == 'e' && 
                                                  context->bufferRecv.at(2 + i) == 'q'  )  return 0;
                                          else if(context->bufferRecv.at(0 + i) == 'e' && 
                                                  context->bufferRecv.at(1 + i) == 'n' && 
                                                  context->bufferRecv.at(2 + i) == 'd'  )  return 1;
                                          else                                             return 2;
                                       };
        int par { ckeckReq() };
        cerr << "ServerRecvClbk: detect type : " << par  <<'\n';
        err_t err { ERR_OK };
        switch(par){
            case 0:
                {
                    cerr << "ServerRecvClbk: send for req\n";
                    Rng                rndn     { GeigerGen2::getRnd() };
                    string             msg      { to_string(rndn.first).append(":").append(to_string(rndn.second)).append(":")
                                                                       .append(to_string(GeigerGen2::getAvailable())).append("\n") };
        
                    context->toSendLen = msg.size() <= context->bufferSend.size() ? msg.size() : context->bufferSend.size();
                    copy_n(msg.data(),  context->toSendLen, context->bufferSend.data());
                    err =  serverSendData(context, context->client_pcb);
                }
            break;
            case 1:
                    cerr << "ServerRecvClbk: close for end\n";
                    err = clientClose(context);
            break;
            default:
                    cerr << "ServerRecvClbk: error\n";
                    err = clientClose(context); 
        } 
    } 

    cerr << "ServerRecvClbk : end \n";
    return err;
}

void GeigerGen2NetworkLayer::serverErrClbk(void *ctx, err_t err)  noexcept{
    cerr << "ServerErrClbk\n";
    if(err != ERR_ABRT) {
        cerr << "ServerErrClbk : " << err << '\n';
        serverResult(ctx, err);
    }
}
  
err_t GeigerGen2NetworkLayer::serverAccept(void *ctx, TcpPcb *client_pcb, err_t err)  noexcept{
    Context *context { static_cast<Context*>(ctx)};
    cerr << "ServerErrClbk\n";
    if(err != ERR_OK || client_pcb == nullptr) {
        cerr << "ServerErrClbk: Error: accept\n";
        clientResult(context, err);
        return ERR_VAL;
    }
    cerr << "ServerErrClbk: Client connected\n";

    context->client_pcb = client_pcb;
    tcp_arg(client_pcb, context);
    tcp_sent(client_pcb, serverSentClbk);
    tcp_recv(client_pcb, serverRecvClbk);
    tcp_err(client_pcb, serverErrClbk);

    string             msg      { "ready\n" };
    context->toSendLen = msg.size() <= context->bufferSend.size() ? msg.size() : context->bufferSend.size();
    copy_n(msg.data(),  context->toSendLen, context->bufferSend.data());
    return serverSendData(context, context->client_pcb);
}

int GeigerGen2NetworkLayer::service(void) noexcept{
    cerr << "Service\n";
    TcpPcb *pcb { tcp_new_ip_type(IPADDR_TYPE_ANY) };
    if(!pcb){
        cerr << "Service : Error: pcb creation\n";
        serverResult(&context, -1);
        return 1;
    }
    ip_set_option(pcb, SOF_REUSEADDR); 

    if(err_t err { tcp_bind(pcb, nullptr, TCP_PORT) }; err != ERR_OK){
        cerr << "Service : Error: bind to port : " <<  TCP_PORT << '\n';
        serverResult(&context, -1);
        return 1;
    }

    context.server_pcb = tcp_listen_with_backlog(pcb, 1);
    if(!context.server_pcb) {
        cerr <<  "Service : Error: listen\n";
        if(pcb) tcp_close(pcb);
        serverResult(&context, -1);
        return 1;
    }
    tcp_arg(context.server_pcb, &context);

    for(;;){
        tcp_accept(context.server_pcb, serverAccept);
        sleep_ms(50);
    }
}

} // End namespace