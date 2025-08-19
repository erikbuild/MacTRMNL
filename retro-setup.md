## Setup

https://github.com/autc04/Retro68
```
git clone git@github.com:autc04/Retro68.git
cd Retro68
git submodule update --init
brew install boost cmake gmp mpfr libmpc bison texinfo
```

[BOOST 1.89 has a bug](https://github.com/EttusResearch/uhd/issues/869)
`brew uninstall boost`
`brew install boost@1.85`
Homebrew doesn't link alternate formulae into /opt/homebrew/lib and /opt/homebrew/include so...
`export LDFLAGS="-L/opt/homebrew/opt/boost@1.85/lib"`
`export CPPFLAGS="-I/opt/homebrew/opt/boost@1.85/include"`

OR we can force the symlink (since I don't care about using it elsewhere?): `brew link boost@1.85 --force --overwrite`

Install the universal interfaces from: 
One of the most easily found downloads is the MPW 3.5 Golden Master release, usually in a file named MPW-GM.img.bin or mpw-gm.img_.bin. At the time of this writing, this can be found at:

http://macintoshgarden.org/apps/macintosh-programmers-workshop
https://www.macintoshrepository.org/1360-macintosh-programmer-s-workshop-mpw-3-0-to-3-5
https://staticky.com/mirrors/ftp.apple.com/developer/Tool_Chest/Core_Mac_OS_Tools/MPW_etc./MPW-GM_Images/MPW-GM.img.bin

Copy "DebuggingLibraries/", "Interfaces/", "Libraries/", and "RuntimeLibraries/" into Retro68/InterfacesAndLibraries/

Once you have all the prerequisites, execute these commands from the top level of the Retro68 directory:

mkdir ../Retro68-build
cd ../Retro68-build
../Retro68/build-toolchain.bash

The toolchain will be installed in the "toolchain" directory inside the build directory. All the commands are in toolchain/bin, so you might want to add that to your PATH.

Add: `/Users/erik/Code/Retro68-build/toolchain/bin` to path

Copied the LaunchAPPL.cfg sample into project and edited to used the "shared" launch method and put the path to my BasiliskII shared folder.

## Building


## Running
Launch command: `LaunchAPPL -e shared MacTRMNL.APPL`