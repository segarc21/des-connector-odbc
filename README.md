# DESODBC - DES Connector/ODBC Driver

DESODBC is an ODBC Driver of Datalog Educational System (DES) (https://des.sourceforge.io/), an open-source database management system developed and maintained by Fernando Saenz-Perez. This project is a GPLv2 derivate work of MySQL Connector/ODBC Driver 9.0.0, the ODBC Driver for MySQL ([mysql-connector-odbc](https://github.com/mysql/mysql-connector-odbc)). The modifications that have resulted in this final work have been done by Sergio Miguel Garcia Jimenez (https://github.com/segarc21). Attributions regarding original authorship and modifications are properly placed in the source files.

Binaries ```desodbc-installer``` correspond to the DESODBC installer; ```w``` corresponds to the Unicode and ```a``` to the ANSI version of the driver; ```S``` to the setup libraries.

The released binaries have been compiled in Windows 10 22H2, Debian 10.10.0 (GLIBC 2.28) and macOS Big Sur 11.0.1.

## Licensing

Please refer to the MySQL Connector/ODBC Driver files README and LICENSE available in this repository and their [Legal Notices in documentation](https://dev.mysql.com/doc/relnotes/connector-odbc/en/preface.html) for further details.

## Install

DESODBC can be installed from pre-compiled packages available in [the GitHub Releases page](https://github.com/segarc21/des-connector-odbc/releases).

Once we have the binaries, we have the possibility to install the driver and the data sources by hand, modifying the ```odbc.ini``` and ```odbcinst.ini``` files. For a fancier way of installing, we have reused the MySQL Connector/ODBC Driver ```myodbc-installer```, which in the DESODBC suite corresponds to the ```desodbc-installer``` binary. This simple tool provides instruction on how to install, modify or remove drivers and data sources. Example of a quick installation of a driver and a data source:

### Windows

Prerrequisites: Visual C++ Redistributables for Visual Studio (Visual C++ Runtime 2015, 2017).

In an admin-privileged session in the command line, execute:
```
> desodbc-installer.exe -d -a -n "DESODBC ANSI Driver" -t "DRIVER=C:\Users\segarc21\desodbca.dll;SETUP=C:\Users\segarc21\desodbcS.dll"
Success: Usage count is 1
> desodbc-installer.exe -s -a -n "DESODBC Data Source" -t "DRIVER=DESODBC ANSI Driver;DESCRIPTION=New data source;DES_EXEC=C:\des\des.exe;DES_WORKING_DIR=C:\des"
Success
```

### GNU/Linux

Prerrequisites: unixODBC 2.2.14 or later, C++ runtime libraries (```libstdc++```). If you want to use the GTK2 or GTK3 setup libraries, install the correspoding GTK versions.

```
> sudo ./desodbc-installer -d -a -n "DESODBC ANSI Driver" -t "DRIVER=/home/segarc21/desodbca.so;SETUP=/home/segarc21/desodbcS.so"
Success: Usage count is 1
> sudo ./desodbc-installer -s -a -n "DESODBC Data Source" -t "DRIVER=DESODBC ANSI Driver;DESCRIPTION=New data source;DES_EXEC=/home/segarc21/des/des;DES_WORKING_DIR=/home/segarc21/des"
Success
```

If errors arise, make sure the environment ```ODBCINI``` points to the correct path that contains ```odbc.ini```.

### macOS

Prerrequisites: unixODBC 2.2.14 or later / iODBC 3.52.12 or later, C++ runtime libraries (```libc++```).

```
> sudo ./desodbc-installer -d -a -n "DESODBC ANSI Driver" -t "DRIVER=/Users/segarc21/desodbca.so;SETUP=/Users/segarc21/desodbcS.so"
Success: Usage count is 1
> sudo ./desodbc-installer -s -a -n "DESODBC Data Source" -t "DRIVER=DESODBC ANSI Driver;DESCRIPTION=New data source;DES_EXEC=/Users/segarc21/des/des;DES_WORKING_DIR=/Users/segarc21/des"
Success
```

If errors arise, make sure the environment ```ODBCINI``` points to the correct path that contains ```odbc.ini```. If the unixODBC version of the installer is used, make sure unixODBC is located in a usual folder like ```/usr/local/opt/unixodbc```. If the iODBC version is used, iODBC should be installed in a usual path like ```/usr/local/iODBC```.

## Compilation from sources

Instructions on compiling are fairly identical to those of MySQL Connector/ODBC Driver, and they can be consulted in [MySQL online manuals](https://dev.mysql.com/doc/connector-odbc/en/connector-odbc-installation.html).

When compiling, the only difference in DESODBC is that there are no dependencies regarding MySQL include files and client libraries nor an installation of OpenSSL is required.

In the following subsections we will provide a simple compilation guide that works in the majority of systems and for most needs.

### Windows

Prerrequisites: Microsoft Data Access SDK, a Visual C++ compiler, CMake.

In the root directory of this project, execute:

```
cmake -G "Visual Studio 17 2022"
```

It might be posible to build this project with other Visual Studio toolchains. Check [MySQL online manuals](https://dev.mysql.com/doc/connector-odbc/en/connector-odbc-installation-source-windows.html).

After it, you can now work in the Visual Studio ```.sln``` project file.

### unixODBC

Prerrequisites are unixODBC 2.2.12 or later, ```unixodbc-dev```, and a working ANSI C++ compiler. In GNU/Linux ```libgtk2.0-dev``` and ```libgtk-3-dev``` is also required.

In the root directory of this project, execute:
```
sudo cmake -G "Unix Makefiles" -DWITH_UNIXODBC=1
sudo make
```

Parameter ```-DCMAKE_BUILD_TYPE``` (with for example```Debug``` or ```Release``` -check possible values in the CMakeCache.txt or MySQL documentation) specifies the type of build desired. Once the binaries are compiled, if an installation within the system is wanted, execute ```make install```.

It may be possible that CMake could not find some files regarding unixODBC. Make sure the environment variable ```ODBC_PATH``` points to the root directory of the unixODBC installation (commonly, ```/usr/local/opt/unixODBC```). An complete example of a compilation at this point would be ```sudo ODBC_PATH=/usr/local/opt/unixodbc cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release```.  In some systems, and especially in macOS with ```ld```, linker errors can arise regarding finding unixODBC libraries. Try to append an attribute in the lines of ```-DODBCINST_LIB_DIR=/usr/local/opt/unixodbc/lib```. Check [MySQL online manuals](https://dev.mysql.com/doc/connector-odbc/en/connector-odbc-installation-source-unix.html) for complete guidelines.

### iODBC
 
Prerrequisites are iODBC 3.52.12 or later and a working ANSI C++ compiler.

In the root directory of this project, execute:
```
sudo cmake -G "Unix Makefiles"
sudo make
```

Parameter ```-DCMAKE_BUILD_TYPE``` (with for example```Debug``` or ```Release``` -check possible values in the CMakeCache.txt or MySQL documentation) specifies the type of build desired. Once the binaries are compilled, if an installation within the system is wanted, execute ```make install```.

It may be possible that CMake could not find some files regarding iODBC. Make sure the environment variable ```ODBC_PATH``` points to the root directory of the iODBC installation (commonly, ```/usr/local/iODBC```). An complete example of a compilation at this point would be ```sudo ODBC_PATH=/usr/local/iODBC cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release```. If linker errors arise try to troubleshoot based on the guidelines provided in the unixODBC installation. Check [MySQL online manuals](https://dev.mysql.com/doc/connector-odbc/en/connector-odbc-installation-source-unix-macos.html) for complete guidelines.