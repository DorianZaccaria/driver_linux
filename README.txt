
==================================  HOWTO  =====================================


  *  Makefile & laucher

Use the folowing rules tu compile and install the project

        * make : to compile source file
        * sudo make install : to install the project
        * sudo insmod xylo_led.ko : to install the module (don't forget to remove
                                    usbtest module or older xylo_led project)

OR

        * run luncher.sh

==================================  SUBJECT  ===================================

Projet "driver Xylo"
====================

Le but du projet est d'ecrire un driver Linux (espace noyau) afin de piloter
les 2 leds de la carte Xylo. Ce driver utilise une entree /sys/bus/usb/driver
ainsi qu'une entre /dev/xylo_led0 comme decrit en cours avec l'exemple
du "panic button".

Le controle de la led s'effectuera par un "masque" correspondant aux leds a allumer,
exemple (pour allumer les 2 leds) :

# echo 3 > /sys/bus/usb/drivers/xylo_led/6-1:1.0/ledmask

  ou bien:

# echo 3 > /dev/xylo_led0


Pour developper ce driver, on utilisera les exemples dÃ©crits dans le repertoire
"FPGA Project - USB-2 (FX2) 1 - Blink leds" des exemples fournis avec la carte.
Ces exemples contiennent le "design" (.rbf) Ã  charger sur la carte ainsi qu'un
fichier .cpp (Windows) permettant de valider le fonctionnement. L'archive .zip fournie
contient egalement la documentation complete de la carte.

Pour les hermetiques Ã  Windaube vous pouvez utiliser le compilateur croise Linux -> Win$
"MinGW", voir http://www.mingw.org. Cependant des versions binaires (.exe) des programmes
de test sont fournis dans le sous-repertoire "CPP".
