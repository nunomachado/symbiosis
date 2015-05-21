#!/bin/bash
args=("$@")

cd ~/work/symbiosisJava/SymbiosisRuntime
java -ea -cp ./bin:../SymbiosisTransformer/SymbiosisRuntime:. pt.tecnico.symbiosis.runtime.Main --main-class $@ --bb-trace ~/work/symbiosisJava/SymbiosisRuntime/traces/${args[0]}.traceBB 
cd ..
