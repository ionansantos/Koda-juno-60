# Visual LX — versão0.1.1

Referência observada: [imagem oficial doTAL-U-NO-LX](https://tal-software.com/images/products/tal-u-no-lx-03.jpg),
aberta na [página do fabricante](https://tal-software.com/products/tal-u-no-lx).

O editor foi redesenhado emJUCE: painel preto/cinza com textura vetorial sutil,
faixas vermelhas e azuis, gruposDCO/HPF/VCF/VCA/ADSR/CHORUS na primeira linha,
LFO/CONTROL na segunda, display verde de presets, master e teclado na parte inferior.
Faders possuem trilhos pretos, escala e caps metálicos; botões têmLEDs vermelhos
e tampas cinza/amarela/laranja. Cabeçalho próprioU-NO-60STUDY e dimensões1000×570.
Não é uma reprodução pixel a pixel, nem usa a imagem comercial como fundo.

Os47IDs e120programas continuam iguais. O tecladoC2–C5 injeta eventosMIDI no
processador; PANIC envia reset no próximo bloco de áudio. MORE dá acesso aos
parâmetros internos. Não houve alteração nos algoritmos do núcleoDSP.

Portamento, arpeggiator, unison eFX adicionais doLX não existem no motorU-No-60
recuperado; seus elementos visuais estão inativos. MIDIlearn também permanece
inativo no editor. Não foram implementadas funções sonoras presumidas.

BuildAU/Standalone passou e a prévia compilada abriu com todos os controles,
presets e teclado. Foi feita inspeção visual da renderização; depois foram
ajustados contraste doMASTER, espaçamento do rodapé e range do teclado.
CTest2/2: [log](../evidence/lx-ctest.log).
Buildfinal: [log](../evidence/lx-build-final.log).

AU0.1.1 instalado em
`~/Library/Audio/Plug-Ins/Components/U-No-60 Study.component`,arm64,
assinaturaad-hoc verificada. A versão anterior foi preservada em
`dist/U-No-60 Study.previous.component` para retorno se necessário.
ReinicieLogic para descarregar o binário antigo e abrir o novo editor.

JUCE/CMake foram usados temporariamente e removidos novamente, junto com os
downloads e a pasta de build. NenhumGhidra ouJDK foi reinstalado.
