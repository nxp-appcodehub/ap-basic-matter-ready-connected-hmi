# Folder content
This folder contains necessary binaries for CBX applications.
The following subfolders are:
 - `bchmi_rt1060_apps/`, containing prebuilt binaries of the Basic Connected HMI application, in different combinations.
 - `k32w0_rcp_app/`, containing the necessary K32W0 binaries for all RCP combinations for DK6 platform.
 - `k32w0_lighting_app/`, containing the necessary K32W0 binaries and pigweed tokenizer databases to exercise a standalone lighting app end node device with logs. 

# bchmi_rt1060_apps
The available combinations are:
 - `BCHMI_OTW_BLE_OT_DK6_MCLI_DISP.hex`, which is the OpenThread only configuration, using OTW for writing the K32W0x1 transceiver, having OT and BLE running on K32W0x1. The K32W0x1 is connected through the DK6 carrier board. Matter CLI and the display are also active in the build.
 - `BCHMI_OTW_OTAP_BLE_OT_DK6_WiFi_NOcred_PTA_MCLI_DISP.hex`, which is the OpenThread + Wi-Fi configuration, using OTW for writing the K32W0x1 transceiver, having OT and BLE running on K32W0x1. The K32W0x1 is connected through the DK6 carrier board. Matter CLI and the display are also active in the build. Wi-Fi credentials are not given at build time, they must be entered in Matter CLI. PTA is also enabled between the Wi-Fi and BLE/OT transciever.
- `BCHMI_WiFi_NOcred_MCLI_DISP.hex`, which is Wi-Fi only configuration. Matter CLI and the display are also active in the build. Wi-Fi credentials are not given at build time, they must be entered in Matter CLI.

# k32w0_rcp_app
These binaries have been generated from `/connectivity_toolbox/matter/third_party/openthread/ot-nxp` folder with the K32W0x1 2.6.11 SDK.

# k32w0_lighting_app
These binaries have been generated using the following build commands:
 - for `chip-k32w0x-light-example-vn-42020`:
```
user@ubuntu: ~/connectivity_toolbox/matter/examples/lighting-app/nxp/k32w/k32w0$ gn gen out/debug --args="k32w0_sdk_root=\"${NXP_K32W0_SDK_ROOT}\" chip_with_OM15082=1 chip_with_ot_cli=0 is_debug=false chip_crypto=\"platform\" chip_with_se05x=0 chip_pw_tokenizer_logging=true chip_software_version=42020"
```
 - for `chip-k32w0x-light-example-vn-42021`:
```
user@ubuntu: ~/connectivity_toolbox/matter/examples/lighting-app/nxp/k32w/k32w0$ gn gen out/debug --args="k32w0_sdk_root=\"${NXP_K32W0_SDK_ROOT}\" chip_with_OM15082=1 chip_with_ot_cli=0 is_debug=false chip_crypto=\"platform\" chip_with_se05x=0 chip_pw_tokenizer_logging=true chip_software_version=42021"
```

In order to check the device logs, user needs to do the following:
```
user@ubuntu: ~/connectivity_toolbox$ cd matter
user@ubuntu: ~/connectivity_toolbox/matter$ source ./scripts/activate.sh
user@ubuntu: ~/connectivity_toolbox/matter$ cd ../binaries/k32w0_lighting_app
user@ubuntu: ~/connectivity_toolbox/binaries/k32w0_lighting_app$ python3 ../../matter/examples/platform/nxp/k32w/k32w0/scripts/detokenizer.py serial -i /dev/ttyACM0 -d chip-k32w0x-light-example-vn-42020-database.bin -o device.txt
```