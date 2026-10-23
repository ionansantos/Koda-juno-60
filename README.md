# KODA-JUNO-60

Código-fonte do sintetizador KODA-JUNO-60 em C++/JUCE.

Versão **0.6.1**, Apple Silicon arm64: barra VT/LOCK/MIDI LEARN/LOCK/CLEAR funcional, além de Portamento, Unison, Delay, Reverb e arpejador. São 113 parâmetros e 120 programas.

- CLEAR restaura o som original do preset atual, apagando os mapeamentos MIDI do preset e preservando os estados de LOCK.
- MIDI LEARN aceita clique no controle sem mudança de valor. Ative, clique no fader/knob, mova um controle que envie MIDI CC e desative MIDI LEARN.
- Menu exibe 73 presets nomeados: os 72 originais e o slot 72 “Arp Magic Synth”, baseado no Dream Arp do TAL-U-NO-LX; slots vazios antigos permanecem compatíveis com sessões e Program Change.
- [Correções 0.6.1 e validação](docs/preset-midi-061.md).
- AU e VST3 instalados nas pastas de plugins do usuário.
- Pacotes e standalone são gerados localmente em `build/` e `dist/`; não são versionados.
- [Como usar VT e MIDI learn; evidências, testes e limites](docs/barra-vt-midi-implementacao.md).
- [Portamento e comparação com TAL](docs/portamento-implementacao.md).
- [Unison, Delay, Reverb e limites de fidelidade](docs/efeitos-implementacao.md).
- [Relatório do arpejador](docs/arpeggiator-implementacao.md).
- [Preset de referência no slot 72 e null-test](docs/reference-preset-null-test.md): o slot existe, mas o resíduo medido é 1,20 dB; não é sample-idêntico.

Treze testes CTest, sanitizers de VT e validação AU da Apple passaram. Foram35 renders VT e27 verificações de áudio aprovadas. Isso confirma resposta dos controles, sem estabelecer timbre ou ADSR absoluto idêntico ao TAL. Reinicie a DAW; backup registrado em `evidence/toolbar/implementation/installation.json`.

As dependências de compilação estão disponíveis localmente: JUCE8.0.15 em
`external/JUCE` e CMake4.4.4 em`.tools`. Para editar e recompilar, execute
`sh tools/build-au.sh --install`. Comandos completos em[compilar.md](docs/compilar.md).
O registro da limpeza anterior permanece em
[dependency-cleanup.json](evidence/dependency-cleanup.json).

## Conteúdo do repositório

Código, testes, presets, documentação e relatórios de análise são mantidos.
`build/`, `dist/`, `.tools/` e `external/` são locais e ignorados pelo Git.
Capturas brutas `.raw`/`.f32` e fatias de executáveis `.arm64` foram removidas;
os relatórios históricos podem mencionar esses arquivos. Métricas e resultados
registrados foram preservados. O banco `evidence/factory-bank.bin`, usado pelo
teste de presets, permanece no projeto.

Para uma cópia nova, instale CMake 3.22+ e obtenha o JUCE 8.0.15:

```sh
git clone --depth 1 --branch 8.0.15 https://github.com/juce-framework/JUCE.git external/JUCE
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build --output-on-failure
```

Binários de distribuição devem ser anexados às Releases do GitHub.
As ferramentas REA/Ghidra são opcionais para pesquisa e não são necessárias
para compilar o sintetizador.


## Correções confirmadas no contexto

- O executável contém **120 programas**, incluindo72 com nomes específicos e48
  registros `default`; não foram encontrados128 registros nesse banco.
- O editor possui **38 controles**:20 sliders,2knobs,13botões,1switch de quatro
  estados e2menus. Painel original695×350pixels.
- Há47slots de parâmetros; alguns nomes originais são literalmente `Param`.
  O clone identifica os aliases e mantém todos os slots, inclusive reservados.
- A cadeia observada difere da cadeia conceitual solicitada: processamento por
  voz a2×, LPF/VCA, decimação e **HPF após a soma**, antes do chorus.
- O motor U-No-60 inicialmente analisado não estabelecia arpejador. A versão 0.3.0 acrescenta um módulo independente, orientado pela análise do TAL-U-NO-LX 5.1.5 e por comparação de áudio real. Veja o relatório da implementação.

## Evidências e inventários

| Pedido | Resultado e evidência |
| --- | --- |
| Lista completa dos parâmetros | [Tabela dos47slots, ranges, defaults, unidades e aliases](docs/parameters.md); [CSV](evidence/parameters/parameters.csv); [JSON](evidence/parameters/parameters.json) |
| Editor/layout | [Contagem, helpers e posições](docs/editor.md); [layout JSON](evidence/parameters/editor-layout.json) |
| Presets/classes | [Extração, estrutura dos registros e diagrama de classes](docs/presets.md); [120nomes CSV](evidence/preset-names.csv); [valores e mapas completos](evidence/factory-presets.json) |
| DSP com constantes | [Relatório DCO/LPF/envelope/LFO/FIR/HPF/chorus](evidence/dsp/README.md); [constantes recuperadas](evidence/dsp/constants.json) |
| Endereços/pseudocódigo | [Índice de Evidence IDs](evidence/INDEX.md); JSON/txt de REA completos em evidence/dsp e evidence/parameters |
| Testes executados | [Log](evidence/validation.txt); [renderização do clone](evidence/startup-demo.wav) |

O original possui slices PowerPC e i386. Foi analisada estaticamente a fatia
i386 em Ghidra12.1.4 através do REA. SHA256 do executável universal:
`90f737e5028179abb0dfbe2b900f5b30f584ea51a5c3e076f3759e8ac67cebd8`;
SHA256 da fatia:
`b46668c96111476a3bde38e3367e8ca323178070ced25a51f47f9dd51dc65359`.
A fatia PowerPC não foi decompilada; não houve execução do VST original.
Pseudocódigo recuperado não é código-fonte original.

## Projeto em etapas

1. **Estrutura/CMake** — [CMakeLists.txt](CMakeLists.txt) cria bibliotecaDSP,
   renderer, testes e formatosAU/VST3/Standalone. O formatoAU foi compilado e instalado.
2. **AudioProcessor/parâmetros** — [PluginProcessor.h](Source/PluginProcessor.h),
   [PluginProcessor.cpp](Source/PluginProcessor.cpp) e
   [Parameters.h](Source/Parameters.h). APVTS,47IDs estáveis, automação, MIDI por
   posição de sample, estados e mudança de programa.
3. **DCO e sub/noise/PWM** — `Oscillator` em
   [SynthDSP.cpp](Source/DSP/SynthDSP.cpp), com tabela de correção extraída em
   [BlepTable.h](Source/DSP/BlepTable.h). A fórmula de pitch e o processamento2×
   preservam peculiaridades observadas.
4. **LPF, envelopes e VCA** — `Filter`, `Envelope`, `renderVoice` no mesmo núcleo;
   tradução das constantes e estados observados. Modo CPU alterna filtro por
   voz/compartilhado, com lacuna de envelope explicitamente marcada.
5. **FIR e HPF** — `processStereo` e `Highpass`: FIR de decimação, ganho e três
   HPFs em série, seguindo a ordem do binário.
6. **LFO e chorus** — tabelas interpoladas, sample/hold, delay/fade, tempo latched
   no noteOn, quatro linhas de delay/allpass e saída estéreo. O slot sync1 usa
   multiplicador5 apesar de rótulo1/16; o projeto mantém essa peculiaridade.
7. **Editor** — [PluginEditor.cpp](Source/PluginEditor.cpp) e
   [LXLookAndFeel.h](Source/LXLookAndFeel.h) seguem o layout visualLX, preservando
   os parâmetros do motorU-No-60. A versão0.1.0 abriu noLogic; a versão0.1.1 foi
   compilada e inspecionada emStandalone. Não é uma cópia pixel a pixel.
8. **Preset manager** — [PresetManager.h](Source/PresetManager.h) lê os120
   registros/47floats/255entradasMIDI. [FactoryPresets.h](Source/FactoryPresets.h)
   incorpora o banco extraído. O AudioProcessor preserva valores editados ao
   trocar programas e salva os120programas no estado do clone. Importação do
   payload original tem API no manager; ainda falta interface de arquivo no UI.

Não há range físico inventado no APVTS. As callbacks originais de unidade e
display são vazias; os ranges0..1 representam o controle bruto. Os menus27/28
usam índices0..4 e0..12. As fórmulas físicas estabelecidas pelo DSP estão no
relatório; ranges/semântica dos slots reservados são inferidos e marcados.

## Compilar

Com CMake3.22+ e uma cópia local do JUCE8:

```sh
cmake -S . -B build -DJUCE_PATH=/caminho/para/JUCE
cmake --build build --config Release
ctest --test-dir build --output-on-failure
```

Os formatosAU,VST3 eStandalone seguem a
[API CMake oficial do JUCE](https://github.com/juce-framework/JUCE/blob/master/docs/CMake%20API.md).
Os parâmetros e attachments seguem
[AudioProcessorValueTreeState](https://docs.juce.com/master/classjuce_1_1AudioProcessorValueTreeState.html).
A compilação CMake não instala automaticamente. Para recompilar o AU e instalar
na pastaComponents do usuário, use `sh tools/build-au.sh --install`. Reinicie o
Logic após substituir o plugin para carregar o novo binário.

Para testar só o núcleo usando CMake, sem JUCE:

```sh
cmake -S . -B build -DTAL_BUILD_PLUGIN=OFF
cmake --build build
ctest --test-dir build --output-on-failure
```

Sem CMake, os testes já podem ser reproduzidos com clang:

```sh
sh tools/check.sh
```

Renderer independente, semJUCE:

```sh
clang++ -std=c++17 -O2 -I Source Source/DSP/SynthDSP.cpp tools/render.cpp -o /tmp/tal-render
/tmp/tal-render /tmp/startup.wav 0
```

Ele toca MIDI69,velocity100,solta após1s e grava4s em WAV estéreo float32 a44100Hz.
É uma renderização do **clone**, não evidência de execução do original.

## O que falta para afirmar MESMO comportamento

Há comentários `// TODO verificar` junto às lacunas e diferenças conhecidas.
As principais são o caminho de envelope/filtro compartilhado, estados
transitórios/tails, estado global de `rand()` por host/plataforma, proteção de
limites/saída finita, concorrência da notificaçãoJUCE de ProgramChange/CC,
MIDIlearn, importaçãoFXB/FXP/big endian, hitboxes e validação do editor/estado em
host. O protocolo está em [validacao-audio.md](docs/validacao-audio.md).

Os testes comprovam leitura fiel dos bytes do banco e estabilidade dos cenários
renderizados. Não comprovam sample-identidade, resposta analógica idêntica ou
equivalência de todos os caminhos de automação. Nenhum resultado de comparação
com áudio do original foi inventado.
