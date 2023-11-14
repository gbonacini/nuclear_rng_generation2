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

#include "geiger_gen2.hpp"
#include "wifi_credential.hpp"

using geigergen2::GeigerGen2,
      geigergen2::GeigerGen2NetworkLayer,
      std::cerr;

int main(void) {
    const unsigned int  INPUT_PIN      { 31    },
                        VTHRESHOLD     { 2500  },
                        ZERO_THRESHOLD { 100   },
                        MAX_RETRIES    { 3 },
                        GRACE_TIME     { 10000 };

    GeigerGen2* gg2 { GeigerGen2::getInstance(INPUT_PIN, VTHRESHOLD, ZERO_THRESHOLD) };
    gg2->init();
    gg2->detect();

    sleep_ms(5000);

    for(;;){

        if(cyw43_arch_init()) {
            cerr << "Error: WIFI init.\n";
            return 1;
        }

        cyw43_arch_enable_sta_mode();

        cerr << "Connecting to Wi-Fi...\n";
        for(int i{1} ; cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_MIXED_PSK, GRACE_TIME) != 0 ; i++){
            cerr << "Connection attempt: " << i << '\n';
            if(i > MAX_RETRIES){
                cerr << "Error: WIFI connection.\n";
                return 1;
            }
        } 
        
        GeigerGen2NetworkLayer geigerGen2NetworkLayer;
        int ret = geigerGen2NetworkLayer.service();
        cerr << "Network loop exits with: " << ret << "\n";
    
        cyw43_arch_deinit();
        cerr << "Disconnected.\n";
    }

    return 0;
} 