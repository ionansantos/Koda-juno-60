# Compilar após editar

Em uma cópia nova do GitHub, instale CMake 3.22+ no PATH e obtenha o JUCE:

```sh
git clone --depth 1 --branch 8.0.15 https://github.com/juce-framework/JUCE.git external/JUCE
```

As pastas `.tools/` e `external/` não são versionadas. O script abaixo usa o
CMake local quando disponível e o CMake do PATH como alternativa.


Dependências locais recuperadas: JUCE8.0.15 em`external/JUCE` e CMake4.4.4 em
`.tools/cmake-4.4.4-macos-universal/CMake.app/Contents/bin`.
Não é necessário instalar essas ferramentas globalmente nem adicionar aoPATH.

## Comando recomendado

```sh
cd ~/documents/tal-un-no-60-clone
sh tools/build-au.sh --install
```

O script configura Release/arm64, compila oAudioUnit e os testes, executaCTest,
assina localmente o componente, verifica a assinatura e instala em:

`~/Library/Audio/Plug-Ins/Components/KODA-JUNO-60.component`

Feche oLogic antes de substituir o plugin e abra novamente depois, para carregar
o novo binário. Para apenas compilar/testar/assinar, sem instalar:

```sh
sh tools/build-au.sh
```

## Comandos manuais equivalentes

```sh
cd ~/documents/tal-un-no-60-clone
CMAKE_BIN="$PWD/.tools/cmake-4.4.4-macos-universal/CMake.app/Contents/bin/cmake"
CTEST_BIN="$PWD/.tools/cmake-4.4.4-macos-universal/CMake.app/Contents/bin/ctest"

"$CMAKE_BIN" -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_OSX_ARCHITECTURES=arm64 \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0 \
  -DBUILD_TESTING=ON

"$CMAKE_BIN" --build build --target TalUNo60Clone_AU tal_tests --parallel 4
"$CTEST_BIN" --test-dir build --output-on-failure

codesign --force --sign - "build/TalUNo60Clone_artefacts/Release/AU/KODA-JUNO-60.component"
codesign --verify --deep --strict "build/TalUNo60Clone_artefacts/Release/AU/KODA-JUNO-60.component"
ditto "build/TalUNo60Clone_artefacts/Release/AU/KODA-JUNO-60.component" \
  "$HOME/Library/Audio/Plug-Ins/Components/KODA-JUNO-60.component"
```

Depois de editar, oCMake recompila somente os arquivos necessários. OJUCE local
é encontrado automaticamente peloCMakeLists; não há caminho de exemplo para
substituir nesses comandos.

## Aplicativo de prévia

Depois da configuração inicial:

```sh
"$CMAKE_BIN" --build build --target TalUNo60Clone_Standalone --parallel 4
open "build/TalUNo60Clone_artefacts/Release/Standalone/KODA-JUNO-60.app"
```
