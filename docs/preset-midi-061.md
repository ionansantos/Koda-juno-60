# Correções 0.6.1

CLEAR restaura os parâmetros sonoros originais do programa selecionado a partir do banco de fábrica, e arpejador, efeitos, portamento e VT a seus valores iniciais. Ignora os LOCKs para essa ação explícita, preserva seus estados e apaga os mapeamentos MIDI do preset, desativa aprendizado e interrompe notas/caudas. As edições continuam sendo mantidas ao alternar presets até usar CLEAR.

A seleção MIDI agora recebe mouseDown do editor e de seus descendentes, resolve o parâmetro associado ao controle e funciona sem mudança de valor. O caminho antigo de gestos/valores permanece para edição por teclado. Ative MIDI LEARN, clique no controle, mova um knob/fader físico que envie CC, depois desative MIDI LEARN. Tocar uma tecla que envia Note On não estabelece um mapeamento CC.

A visibilidade dos programas pertence ao processador: nomes vazios/default são omitidos do menu e da navegação. Os 120 índices internos permanecem inalterados; um slot antigo selecionado aparece como Init. Slots renomeados aparecem normalmente.

Validação: 14/14 testes CTest, incluindo mouseDown sem alteração de valor seguido por CC, aplicação MIDI, reset com todos os LOCKs ativos apagando mapas, persistência do reset ao retornar ao preset, 73 itens iniciais, navegação circular e slots legados/renomeados. AU/VST3/Standalone arm64 compilados e assinados; captura visual inspecionada. Logs e instalação em evidence/preset-midi-061/. Teclado físico e roteamento MIDI dentro da DAW não foram verificados nesta rodada. O caminho de cutoff ganhou um modo restrito ao slot 72; os demais programas mantêm a curva anterior.
