1. `git clone --depth 1 --recursive --shallow-submodules https://github.com/tyomitch/uefi`
2. inside `edk2\BaseTools\Conf\target.template`, set `TARGET_ARCH` to `X64`, and `TOOL_CHAIN_TAG` to `VS2019`
3. open `VS\NT32.sln`, press F5 to build and run

Author of the original UEFI Programming Tutorial: Nikolay.Bodunov@gmail.com
