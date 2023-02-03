#!/bin/bash

patch_app()
{
    # copy/replace additional/modified SDK files
    if [ -d "fixes" ]; then
        cp -r fixes/* $1
    fi

    # copy lvgl files in repo
    cp -r lvgl/ $1/third_party/nxp/rt_sdk/repo/middleware/
    echo "Copied lvgl folder to  $1\/third_party/nxp/rt_sdk/repo/middleware folder."

    cp lvgl_support.c $1/examples/platform/nxp/rt/rt1060/board/
    cp lvgl_support.h $1/examples/platform/nxp/rt/rt1060/board/
    echo "Copied lvgl support files to RT1060 board folder."

    # apply board patches
    patch -N $1/examples/platform/nxp/rt/rt1060/app/project_include/FreeRTOSConfig.h <FreeRTOSConfig.h.patch || :
    patch -N $1/examples/platform/nxp/rt/rt1060/board/board.c <board.c.patch || :
    patch -N $1/examples/platform/nxp/rt/rt1060/board/hardware_init.c <hardware_init.c.patch || :
    patch -N $1/examples/platform/nxp/rt/rt1060/board/pin_mux.c <pin_mux.c.patch || :
    patch -N $1/examples/platform/nxp/rt/rt1060/board/pin_mux.h <pin_mux.h.patch || :
    patch -N $1/src/lib/shell/streamer_nxp.cpp <streamer_nxp.cpp.patch || :
    patch -N $1/src/platform/nxp/common/Logging.cpp <Logging.cpp.patch || :
    patch -N $1/src/platform/nxp/common/ConnectivityManagerImpl.cpp <ConnectivityManagerImpl.cpp.patch || :
    patch -N $1/src/platform/nxp/common/ConnectivityManagerImpl.h <ConnectivityManagerImpl.h.patch || :
    patch -N $1/src/platform/nxp/common/NetworkCommissioningDriver.h <NetworkCommissioningDriver.h.patch || :
    patch -N $1/src/platform/nxp/common/NetworkCommissioningWiFiDriver.cpp <NetworkCommissioningWiFiDriver.cpp.patch || :
    patch -N $1/src/platform/nxp/rt/rt1060/PlatformManagerImpl.cpp <PlatformManagerImpl.cpp.patch || :
    patch -N $1/third_party/nxp/nxp_sdk.gni <nxp_sdk.gni.patch || :
    patch -N $1/third_party/nxp/rt_sdk/rt1060/rt1060.gni <rt1060.gni.patch || :
    patch -N $1/third_party/nxp/rt_sdk/rt_executable.gni <rt_executable.gni.patch || :
    patch -N $1/third_party/nxp/rt_sdk/rt_sdk.gni <rt_sdk.gni.patch || :
    patch -N $1/third_party/nxp/rt_sdk/BUILD.gn <BUILD_rt_sdk.gn.patch || :
    patch -N $1/third_party/nxp/rt_sdk/rt1060/BUILD.gn <BUILD_rt1060.gn.patch || :
    patch -N $1/third_party/nxp/rt_sdk/repo/middleware/wifi_nxp/wlcmgr/wlan.c <wlan.c.patch || :

    echo "Matter SDK folder \"$1\" has been patched!"
}

main()
{
    if [ $# != 0 ]; then
        echo >&2 "Trailing arguments: $@"
        # 128 for Invalid arguments
        exit 128
    fi

    patch_app ../../matter
    
    cd ../../matter/third_party/nxp/rt_sdk/repo/middleware/wireless/framework/
    
    env -i git checkout 9f693f8471de4305b8a18bc45e6c29858c46a615
    
    echo "Updating framework commit to RT1060 SDK ver. 2.13.0"
    
    cd ../../../../../../../../basic_connected_hmi_app/patch/
}

main "$@"
