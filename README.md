wdled
=====

This is a simple Linux tool to control the LED mode of WD My Passport drives.

For Windows and macOS you can use the official [WD Drive Utilities](https://support.wdc.com/downloads.aspx).

Usage
-----
```
./wdled DEVICE [VALUE]
```
* DEVICE:  
  SCSI device to control (e.g /dev/disk/by-id/usb-WD_My_Passport_...)
* VALUE:  
  LED mode to set ('on' or 'off', 0 or 255)  
  Omit to read current mode  
  Prefix with 'save:' to have the disk remember the LED mode  

Examples
--------
To turn the LED off permanently:
```
wdled /dev/disk/by-id/usb-WD_My_Passport_foo save:off
```

To turn the LED off until next power cycle:
```
wdled /dev/disk/by-id/usb-WD_My_Passport_foo off
```

To get the current LED mode:
```
wdled /dev/disk/by-id/usb-WD_My_Passport_foo
```

Supported Devices
-----------------
* WD My Passport 259D
* WD My Passport 259E
* WD My Passport 259F
* WD My Passport 259A
* WD My Passport 25E1
* WD My Passport 25E2

To prevent sending weird commands to unsuspecting disks, *wdled* has a hardcoded list of known supported devices and will refuse to operate if a disk does not match this list.

To facilitate testing, you can force *wdled* to operate on an unknown disk.

If you determine your device is supported by this tool, please submit a github pull request or issue.

### Bypass supported device check and get LED mode
This should be RELATIVELY safe to run, and will do some sanity checks, if you see something like the following, your disk MAY be supported:  
```
$ wdled /dev/disk/by-id/usb-WD_My_Passport_foo FORCEGET
WARNING: Skipping supported vendor/product checks!
/dev/disk/by-id/usb-WD_My_Passport_foo: WD       My Passport 1337 (rev 1234)
MANUALLY SKIPPED UNSUPPORTED DEVICE CHECK!
LED: current=255 original=255 saved=255
```
If this works, you want to flip the LED mode in WD Drive Utilities, then run the command again and make sure the value has changed as expected.

### Bypass supported device check and set LED mode
**DANGER**: Running this command may **break your disk**, and cause **all your data to be lost**!  
The LED setting is _VERY_ vendor specific, so please only try this on WD My Passport drives or derivatives.  
If you're sure your disk is supported, you may want to try forcing the LED mode set:  
```
$ wdled /dev/disk/by-id/usb-WD_My_Passport_foo FORCESET:off
WARNING: Skipping supported vendor/product checks!
/dev/disk/by-id/usb-WD_My_Passport_foo: WD       My Passport 1337 (rev 1234)
MANUALLY SKIPPED UNSUPPORTED DEVICE CHECK!
LED: current=255 original=255 saved=255
```

Installing (Ubuntu)
-------------------
You can install a pre-built version of *wdled* from an Ubuntu PPA: https://launchpad.net/~jbit.net/+archive/ubuntu/wdled

```
add-apt-repository ppa:jbit.net/wdled
apt install wdled
```

Building
--------
Make sure you have [sg3_utils](http://sg.danny.cz/sg/sg3_utils.html) development files installed.

On Debian and Ubuntu:
```
apt install libsgutils2-dev
``` 
On Fedora/CentOS:
```
yum install sg3_utils-devel
```

Then just run make!
```
make
```

License
-------
Copyright 2020 James Lee (jbit@jbit.net)

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain
     the above copyright notice,
     this list of conditions
     and the following disclaimer.

2. Redistributions in binary form must reproduce
     the above copyright notice,
     this list of conditions
     and the following disclaimer
     in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
