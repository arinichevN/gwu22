#!/bin/bash

APP=gwu22
APP_DBG=`printf "%s_dbg" "$APP"`
INST_DIR=/usr/sbin
CONF_DIR=/etc/controller
CONF_DIR_APP=$CONF_DIR/$APP
PID_DIR=/var/run

#lubuntu
#PSQL_I_DIR=-I/usr/include/postgresql

#xubuntu
PSQL_I_DIR=-I/opt/PostgreSQL/9.5/include 

PSQL_L_DIR=-L/opt/PostgreSQL/9.5/lib

MODE_DEBUG=-DMODE_DEBUG

#PLATFORM=-DPLATFORM_ANY
#PLATFORM=-DPLATFORM_A20
PLATFORM=-DPLATFORM_H3

NONE=-DNONEANDNOTHING

INI_MODE=-DFILE_INI
#INI_MODE=-DDB_INI

function move_bin {
	([ -d $INST_DIR ] || mkdir $INST_DIR) && \
	cp $APP $INST_DIR/$APP && \
	chmod a+x $INST_DIR/$APP && \
	chmod og-w $INST_DIR/$APP && \
	echo "Your $APP executable file: $INST_DIR/$APP";
}

function move_bin_dbg {
	([ -d $INST_DIR ] || mkdir $INST_DIR) && \
	cp $APP_DBG $INST_DIR/$APP_DBG && \
	chmod a+x $INST_DIR/$APP_DBG && \
	chmod og-w $INST_DIR/$APP_DBG && \
	echo "Your $APP executable file for debugging: $INST_DIR/$APP_DBG";
}

function move_conf {
	([ -d $CONF_DIR ] || mkdir $CONF_DIR) && \
	([ -d $CONF_DIR_APP ] || mkdir $CONF_DIR_APP) && \
	cp  main.conf $CONF_DIR_APP && \
	cp  config.tsv $CONF_DIR_APP && \
	cp  device.tsv $CONF_DIR_APP && \
	echo "Your $APP configuration files are here: $CONF_DIR_APP";
}

#your application will run on OS startup
function conf_autostart {
	cp -v starter_init /etc/init.d/$APP && \
	chmod 755 /etc/init.d/$APP && \
	update-rc.d -f $APP remove && \
	update-rc.d $APP defaults 30 && \
	echo "Autostart configured";
}
function build_lib {
	gcc $1 $PLATFORM -c app.c -D_REENTRANT -lpthread && \
	gcc $1 $PLATFORM -c crc.c
	if (test $INI_MODE = -DDB_INI)
	then 
		gcc $1 $PLATFORM -c db.c $PSQL_I_DIR $PSQL_L_DIR -lpq 
		gcc $1 $PLATFORM -c config.c $PSQL_I_DIR $PSQL_L_DIR -lpq
	fi
	gcc $1 $PLATFORM -c gpio.c && \
	gcc $1 $PLATFORM -c timef.c && \
	gcc $1 $PLATFORM -c udp.c && \
	gcc $1 $PLATFORM -c util.c && \
	gcc $1 $PLATFORM -c dht22.c && \
	
	cd acp && \
	gcc $1 $PLATFORM -c main.c && \
	cd ../ && \
	echo "library: making archive..." && \
	rm -f libpac.a
	if (test $INI_MODE = -DDB_INI)
	then
		ar -crv libpac.a app.o crc.o db.o gpio.o timef.o udp.o util.o dht22.o config.o acp/main.o && echo "library: done"
	else
		ar -crv libpac.a app.o crc.o gpio.o timef.o udp.o util.o dht22.o acp/main.o && echo "library: done"
	fi
	echo "library: done"
	rm -f *.o acp/*.o
}
#    1         2
#debug_mode bin_name
function build {
	cd lib && \
	build_lib $1 && \
	cd ../ 
	if (test $INI_MODE = -DDB_INI)
	then
		gcc -D_REENTRANT $1 $PLATFORM $INI_MODE $PSQL_I_DIR $PSQL_L_DIR main.c -o $2 -lpthread -lpq -L./lib -lpac && echo "Application successfully compiled. Launch command: sudo ./"$2
	else
		gcc -D_REENTRANT $1 $PLATFORM $INI_MODE main.c -o $2 -lpthread -L./lib -lpac && echo "Application successfully compiled. Launch command: sudo ./"$2
	fi
}

function full {
	build $NONE $APP && \
	build $MODE_DEBUG $APP_DBG  && \
	move_bin && move_bin_dbg && move_conf && conf_autostart
}

function part_debug {
	build $MODE_DEBUG $APP_DBG
}
function uninstall {
	pkill -F $PID_DIR/$APP.pid --signal 9
	update-rc.d -f $APP remove
	rm -f $INST_DIR/$APP
	rm -f $INST_DIR/$APP_DBG
	rm -rf $CONF_DIR_APP
}

f=$1
${f}