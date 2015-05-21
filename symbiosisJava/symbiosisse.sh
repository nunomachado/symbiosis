#!/bin/bash
cd ~/work/symbiosisJava/jpf-symbiosis
java -Xmx1500m -jar /home/symbiosis/work/symbiosisJava/jpf-core/build/RunJPF.jar +shell.port=4242 /home/symbiosis/work/symbiosisJava/jpf-symbiosis/src/examples/${1}.jpf
