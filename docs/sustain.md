# KODA-JUNO-60 — sustain MIDI / versão0.2.0

O produto, AudioProcessor, cabeçalho da interface, AudioUnit e app independente
agora se chamam **KODA-JUNO-60**. Os IDs dos47parâmetros, a árvore de estado e a
identidadeAU foram mantidos para permitir que projetos existentes recuperem o
estado. O nome exibido do fabricante éKODA. O instalador move o componente antigo
para um backup emdist e deixa somenteKODA-JUNO-60 na pastaComponents.

## Pedal

MIDI1.0: **CC64, valores0..63 soltam o pedal;64..127 pressionam**.
Fonte: [MIDI Association — Control Change](https://midi.org/midi-1-0-control-change-messages).

O pedal é controlado separadamente nos16canaisMIDI. Ao receberNoteOff com pedal
pressionado, a voz permanece com gate ligado. Ao liberar o pedal, somente notas
cujas teclas já foram soltas iniciam o release. Teclas ainda pressionadas continuam
soando. Cada voz mantém canal e estado da tecla; a realocação de uma voz limpa essa
associação, evitando release incorreto após roubo ou repetição de nota.

CC64 é tratado antes do mapa de automação dos presets. CC121 libera o pedal e
reseta controllers; CC123 desliga teclas respeitando o pedal; CC120 corta as vozes
do canal. PANIC e reset limpam notas e estados dos pedais. O pitchwheel continua
global, uma limitação anterior independente do suporte de sustain por canal.

A luz**SUSTAIN** no cabeçalho acende quando existe pedal pressionado em algum
canal. Ela permite conferir o recebimento deCC64. NoStandalone, habilite a
controladora nas opções de áudio/MIDI para receber seus eventos.

O pedal mantém notas; o som continua seguindoADSR, filtro echorus do preset.
Ele não modifica o parâmetroAMPSUSTAIN: um envelope com sustain zero ainda pode
decair até silêncio. Esse é o comportamento de envelope, não falta de recepção
do pedal. O suporte implementado é sustain on/offMIDI1.0, não half-pedal.

## Verificação

- DSP comASan/UBSan: limites63/64, pedal-up, tecla segurada, retrigger, mesmo
  pitch em canais diferentes, roubo de voz, resets echorus.
- AudioProcessor comMidiBuffer real doJUCE: CC64 com offset de sample, note-off
  velocityzero, separação por canal, CC121, PANIC e luzSUSTAIN.
- Regressão dos120presets e quatro sample rates.

Os testes de software não substituem uma verificação com o seu modelo físico
de controladora/pedal. Logs: [sanitizers](../evidence/koda-sustain-sanitizers.log),
[build eCTest](../evidence/koda-final-build.log).

## Compilar e abrir

```sh
cd ~/documents/tal-un-no-60-clone
sh tools/build-au.sh --install
.tools/cmake-4.4.4-macos-universal/CMake.app/Contents/bin/cmake --build build --target TalUNo60Clone_Standalone --parallel 4
open "build/TalUNo60Clone_artefacts/Release/Standalone/KODA-JUNO-60.app"
```

AU: `~/Library/Audio/Plug-Ins/Components/KODA-JUNO-60.component`.
Reinicie oLogic após atualizar oAU para descarregar o binário anterior.
