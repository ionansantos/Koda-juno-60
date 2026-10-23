#!/bin/sh
# Ferramentas opcionais de pesquisa, instaladas fora do projeto.
set -eu
: "${GHIDRA_INSTALL_DIR:=$HOME/Applications/ghidra_12.1.4_PUBLIC}"
: "${JAVA_HOME:=$HOME/Library/Java/JavaVirtualMachines/temurin-21.jdk/Contents/Home}"
export GHIDRA_INSTALL_DIR JAVA_HOME
exec npx -y rea-agents@6.4.0 "$@"
