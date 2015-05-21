#!/bin/bash
cd ~/work/symbiosisJava/SymbiosisTransformer
java -Xmx1500m -cp ./bin:./lib/jasminclasses-2.4.0.jar:./lib/sootclasses-2.4.0.mine.jar:./lib/polyglotclasses-1.3.5.jar:../../Tests/JavaTests:../../Tests/JavaTests/cache4j/bin pt.tecnico.symbiosis.transformer.SymbiosisTransformer $1
cd ..
