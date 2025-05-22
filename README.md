# DESODBC - DES Connector/ODBC Driver

DESODBC is an ODBC Driver of Datalog Educational System (DES) (https://des.sourceforge.io/), an open-source database management system developed and maintained by Fernando Saenz-Perez. This project is a GPLv2 derivate work of MySQL Connector/ODBC Driver 9.0.0, the ODBC Driver for MySQL ([mysql-connector-odbc](https://github.com/mysql/mysql-connector-odbc)). The modifications that have resulted in this final work have been done by Sergio Miguel Garcia Jimenez (https://github.com/segarc21). Attributions regarding original authorship and modifications are properly placed in the source files.

Binaries ```desodbc-installer``` correspond to the DESODBC installer; ```w``` corresponds to the Unicode and ```a``` to the ANSI version of the driver; ```S``` to the setup libraries.

## Licensing

Please refer to the MySQL Connector/ODBC Driver files README and LICENSE available in this repository and their [Legal Notices in documentation](https://dev.mysql.com/doc/relnotes/connector-odbc/en/preface.html) for further details.

## Install

DESODBC can be installed from pre-compiled packages available in [the GitHub Releases page](https://github.com/segarc21/des-connector-odbc/releases).

Once we have the binaries, we have the possibility to install the driver and the data sources by hand, modifying the ```odbc.ini``` and ```odbcinst.ini``` files. For a fancier way of installing, we have reused the MySQL Connector/ODBC Driver ```myodbc-installer```, which in the DESODBC suite corresponds to the ```desodbc-installer``` binary. This simple tool provides instruction on how to install, modify or remove drivers and data sources. Example of a quick installation of a driver and a data source:

### Windows
In an admin-privileged session in the command line, execute:
```
> desodbc-installer.exe -d -a -n "DESODBC ANSI Driver" -t "DRIVER=C:\Users\segarc21\desodbca.dll;SETUP=C:\Users\segarc21\desodbcS.dll"
Success: Usage count is 1
> desodbc-installer.exe -s -a -n "DESODBC Data Source" -t "DRIVER=DESODBC ANSI Driver;DESCRIPTION=New data source;DES_EXEC=C:\des\des.exe;DES_WORKING_DIR=C:\des"
Success
```

### GNU/Linux
```
> sudo ./desodbc-installer -d -a -n "DESODBC ANSI Driver" -t "DRIVER=/home/segarc21/desodbca.so;SETUP=/home/segarc21/desodbcS.so"
Success: Usage count is 1
> sudo ./desodbc-installer -s -a -n "DESODBC Data Source" -t "DRIVER=DESODBC ANSI Driver;DESCRIPTION=New data source;DES_EXEC=/home/segarc21/des/des;DES_WORKING_DIR=/home/segarc21/des"
Success
```

### macOS
```
> sudo ./desodbc-installer -d -a -n "DESODBC ANSI Driver" -t "DRIVER=/Users/segarc21/desodbca.so;SETUP=/Users/segarc21/desodbcS.so"
Success: Usage count is 1
> sudo ./desodbc-installer -s -a -n "DESODBC Data Source" -t "DRIVER=DESODBC ANSI Driver;DESCRIPTION=New data source;DES_EXEC=/Users/segarc21/des/des;DES_WORKING_DIR=/Users/segarc21/des"
Success
```

## Compilation from sources

Instructions on compiling are fairly identical to those of MySQL Connector/ODBC Driver, and they can be consulted in [MySQL online manuals](https://dev.mysql.com/doc/connector-odbc/en/connector-odbc-installation.html).

When compiling, the only difference in DESODBC is that there are no dependencies regarding MySQL include files and client libraries nor an installation of OpenSSL is required.

In the following subsections we will provide a simple compilation guide that works in the majority of systems and for most needs.

### Windows
In the root directory of this project, execute:
```
cmake -G "Visual Studio 17 2022"
```
After it, you can now work in the Visual Studio ```.sln``` file.

### unixODBC
In the root directory of this project, execute:
```
sudo cmake -G "Unix Makefiles" -DWITH_UNIXODBC=1
sudo make
```
In some systems, and especially in macOS with ```ld```, linker errors can arise regarding finding unixODBC. Review the variables in ```CMakeCache.txt``` and append the neccessary additional parameter (for example, ```-DODBCINST_LIB_DIR=/usr/local/lib```).

### iODBC
In the root directory of this project, execute:
```
sudo cmake -G "Unix Makefiles"
sudo make
```
It may be possible that CMake could not find some files regarding iODBC. Review the variables in ```CMakeCache.txt``` and append the neccessary additional parameter (for example, ```-DODBC_CONFIG=/usr/local/iODBC/bin/iodbc-config```).
