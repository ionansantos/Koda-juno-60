# MIDI LEARN / LOCK / CLEAR — evidência TAL

Alvo: TAL-U-NO-LX 5.1.5 instalado, AU ARM64 em macOS. SHA-256 universal `99bcc10a28984d52e46299f8369b7a3cb0648ae99358ec3e8d5628d5f113aba6`; fatia ARM64 `f42ba995938a5df3a0beed6d28c214b80207c919f692306650222a32b1ef55e0`.

## Comportamento observado no AU real

[Resultados e identidade](../evidence/toolbar-midi/runtime.json), [probe](../tools/toolbar-midi-probe.cpp). O probe instancia somente o AU explicitamente selecionado, importa `jucePluginState` através de ClassInfo, envia MIDI e renderiza quatro blocos de 256 amostras a 44,1 kHz. Não abre dispositivo de áudio, não altera o plugin instalado nem salva configurações. Um ciclo de mensagens de 0,1 s permite ao TAL despachar a atualização de parâmetros.

- Mapa CC74→filtercutoff, cutoff inicial 0,25: valor MIDI127 resulta em cutoff1,0 nos canais1,2 e16. Portanto o AU real aceita os três canais; o código interno isolado não basta para afirmar limitação ao canal1.
- Valor MIDI64 resulta em `0.5039370059967041`, consistente com `valor/127`.
- CC64 e CC123 também podem atualizar o parâmetro mapeado. Isso não cancela a função musical de sustain ou All Notes Off.
- Duas entradas para o mesmo CC74, primeiro cutoff e depois ressonância: cutoff permanece0,25 e ressonância passa a1,0. O último destino substitui o primeiro.

O mapa foi importado deliberadamente em uma instância isolada; o teste **não** executa o gesto de MIDI LEARN na interface, nem troca presets com LOCK. Não prova fluxo de interação visual idêntico.

## Derivação do código com REA

REA/Ghidra 12.1.4, resultados históricos preservados, sem nova sessão ou reanálise do binário:

| Origem | Evidence ID | Conclusão derivada |
|---|---|---|
| [SynthEngine::parameterChanged, 0x53238](../evidence/arpeggiator/rea-20261018-unrestricted/07-parameters.cpp.txt) | `ev_088cae064f597d74469a9ff0010beef35d7d980fc1cd55a3a37cf985df88dc29` | LEARN ativa booleano +0x578. CLEAR esvazia 128 células, independentemente de LOCK. MIDI LOCK escreve booleano +0xc1. Último parâmetro alterado, exceto MidiLearn, torna-se destino +0x580. |
| [TalCore::processBlock, 0x276ec](../evidence/arpeggiator/rea-20261018-playback/10-process.cpp.txt) | `ev_71e37182bed5a7abc82d87ad3081854b53faf5cccbcbefb7c91fb58f45379fa0` | Tabela indexada pelo númeroCC, uma string destino por célula em +0x178. LEARN escreve a célula e o mesmo evento aplica valor/127. Não há desligamento de LEARN nesse ramo. Vários CC podem apontar ao mesmo parâmetro. |
| [TalCore::setStateInformationFromXml, 0x32728](../evidence/lx-effects/rea-20261019/026-state-load.cpp.txt) | `ev_4ccc6f5546b68fde0633921899eefd0e6001a3e83ba725a8faac97f9aa0141d6` | Formato atual `midimap` guarda `param` por string e `controllernumber`. Importação é protegida por MIDI LOCK quando há nome de preset; nome vazio permite restauração de estado. Não há campo de canal na célula. |
| [Construção e listeners](../evidence/lx-effects/rea-20261019/035-engine-construction.cpp.txt) | `ev_47214377d6aa111345191a6efc3a801a30a06ee9e7ab4c133ad914f6d0ba28d9` | MidiLearn, MidiLock e MidiClear usam o grupo global de parâmetros, separado dos parâmetros de programa. |

Pseudocódigo é derivação, não fonte original nem execução. No ramo interno de processBlock há filtro de canal dependente de MPE; a observação AU acima tem precedência sobre inferir o comportamento externo a partir desse ramo.

## Decisões de integração KODA

Canal independente, um destino por CC, vários CC por destino; preservar as funções musicais dos CC antes de aplicar o mapa. LOCK protege o mapa durante a troca de preset; não bloqueia aprendizagem nem CLEAR. LEARN é estado transitório da interface. O KODA exclui os próprios botões administrativos dos destinos de aprendizagem, evitando aprender CLEAR/LOCK e criar operações recursivas; isso é uma decisão explícita de produto, pois o setter genérico do TAL exclui somente MidiLearn.

Permanecem sem observação direta nesta investigação: gesto de aprendizagem no editor TAL, menu contextual por parâmetro, persistência entre processos e restauração de LOCK pelo host. Não apresentar esses pontos como equivalência medida.
