# Arpejador TAL-U-NO-LX 5.1.5 e aplicação ao KODA-JUNO-60

> Implementação posterior: [KODA 0.3.0, controles e validação](arpeggiator-implementacao.md). Este documento preserva os achados e as limitações da etapa de análise.

Investigação de 18/10/2026. Escopo: analisar a seção da imagem para orientar a implementação; o motor e o plugin instalado do KODA não foram alterados. A imagem fornecida é referência visual, não instrução de execução.

## Identidade e método

Alvo instalado: `/Library/Audio/Plug-Ins/Components/TAL-U-NO-LX-V2.component/Contents/MacOS/TAL-U-NO-LX-V2`, versão declarada 5.1.5. SHA-256 universal `99bcc10a28984d52e46299f8369b7a3cb0648ae99358ec3e8d5628d5f113aba6`. Fatia ARM64 previamente derivada: `../evidence/arpeggiator/TAL-U-NO-LX-V2.arm64`, SHA-256 `f42ba995938a5df3a0beed6d28c214b80207c919f692306650222a32b1ef55e0`.

REA local com Ghidra 12.1.4 recuperou pseudocódigo e assembly. Isso é interpretação estática de código compilado, não código-fonte original nem observação da execução. Os bloqueios anteriores de socket e inspeção de processos não ocorreram nesta sessão. Não foi necessário alterar o registro global do MCP. As duas sessões nativas foram fechadas com sucesso.

Fontes locais completas: [índice de Evidence](../evidence/arpeggiator/INDEX.md). A segunda sessão também foi exportada em `../evidence/arpeggiator/rea-20261018-playback/evidence-bundle.json`. A primeira terminou após um dossier exceder o transporte MCP; os quatro dossiers anteriores estão salvos, mas o dossier excedente não foi exportado. O fluxo correspondente foi recuperado na segunda sessão por `procedure_pseudo_code`.

## Como o código organiza o arpejador

Observado: há uma coleção das notas pressionadas, um buffer de trabalho e uma sequência de reprodução. Cada registro copiado tem 16 bytes. O campo de altura fica no deslocamento +4 e a expansão altera somente esse campo; os demais dados do registro acompanham a nota. A interpretação dos quatro campos como identidade original, altura de reprodução, canal e velocidade é derivada do fluxo até VoiceManager; a assinatura C++ reconstruída pelo decompilador não deve ser copiada literalmente.

`calculateSortedArpeggiatorNotes` (0x4741c) copia as notas, ordena conforme o modo, expande oitavas e monta a sequência. No modo normal, valor interno de direção 0 ordena descendente, 1 cria subida/descida e 2 ordena ascendente. Os comparadores de 0x478ac e 0x48268 corroboram a ordenação pela altura.

`addOctaveArray` (0x48c24) usa range interno 0, 1, 2: total de uma, duas ou três oitavas. Copia blocos completos de notas, acrescentando 12 e 24 semitons. Na direção descendente, as cópias ficam abaixo das notas originais; nas outras, acima. Não há clamp MIDI nessa rotina: o tratamento final de alturas fora de 0..127 ainda precisa ser verificado no consumidor.

Exemplos derivados do código, modo especial normal e range de uma oitava:

| Modo | Acorde C4–E4–G4 |
| --- | --- |
| UP | C4, E4, G4, C4… |
| DOWN | G4, E4, C4, G4… |
| UP&DOWN | C4, E4, G4, E4, C4… |

UP&DOWN acrescenta o trecho inverso sem as duas extremidades para sequências de três notas ou mais; com duas notas, alterna diretamente. O modo especial interno 3 acrescenta a sequência reversa inteira, repetindo extremos. O valor 2 seleciona o próximo índice com gerador pseudoaleatório e tenta evitar repetir o índice atual. Os nomes exatos das opções do menu NORMAL e os valores de automação desse menu não foram estabelecidos.

## Tempo e disparo

`setArpRate` (0x43a58) calcula, para valor normalizado x:

```
frequência ≈ 1.2631578 * exp(2.9957323 * x) - 0.26315784
incremento de fase = frequência / sampleRate
```

Os extremos são aproximadamente 1 e 25 Hz; o centro fica perto de 5,39 Hz. Portanto, o fader tem curva exponencial e a escala visual 0–10 não representa diretamente BPM ou Hz.

Com SYNC, o índice é floor(17*x + 0.5). Foram recuperados 18 rótulos, incluindo notas pontuadas e tercinas:

`1/2, 1/4., 1/2T, 1/4, 1/8., 1/4T, 1/8, 1/16., 1/8T, 1/16, 1/32., 1/16T, 1/32, 1/64., 1/32T, 1/64, 1/128., 1/64T`.

O incremento é `4 * multiplicador * BPM / (60 * sampleRate)`. Há uma particularidade confirmada também no assembly: o rótulo 1/64 compartilha multiplicador 3 com 1/32T, e 1/128. usa 4. Não substituir esses números por valores musicalmente presumidos nem declarar bug audível sem teste no TAL. O manual público resume menos divisões que esta versão local.

`TalCore::processBlock` (0x276ec) processa eventos e avança o arpejador dentro do fluxo de áudio. A cada passo, reseta as vozes por `safeResetByAudioThread`, envia a nota escolhida ao VoiceManager e avança o índice. Esse caminho não comprova gate fixo de 50%; a relação completa entre duração, envelope e reset continua não verificada.

| Fonte | Evidência estática |
| --- | --- |
| Livre | Acumula fase por amostra e avança ao cruzar 1. |
| HOST | Usa BPM e posição musical PPQ; deriva a fase da posição. Detecta transição perto do fim para o começo do ciclo. Ler somente BPM no KODA não reproduz esse alinhamento. |
| MIDI CLK | Trata F8 e conta pulsos módulo int(6/multiplicador); FA e FB zeram o contador. A política completa para FC/Stop ainda não foi demonstrada. |
| EXT16 | Com arp e SYNC ativos, note-on com velocidade não zero no canal MIDI 16 solicita um passo; note-off correspondente é consumido. As notas do canal 16 servem como gatilho nesse caminho. |

Quando clocks externos e SYNC estão ativos, o avanço usa o sinal de gatilho; fora dessa combinação, usa o acumulador. A prioridade musical quando várias fontes são ativadas ao mesmo tempo requer teste controlado.

## HOLD, LOCK e teclas

Observado em `setNoteOn` (0x548b0): com arp desligado, a nota segue para o gerenciador de vozes; com arp ligado, entra na coleção e reconstrói a sequência. A coleção deduplica pela altura nesse caminho, sem considerar canal na comparação. Não assumir que reproduzir esse detalhe seja desejável para o KODA, cujo sustain atual tem semântica por canal.

Observado em `setNoteOff` (0x54b2c): remove a nota da coleção física. Com HOLD ligado, conserva a sequência de reprodução. Sem HOLD, recalcula; ao soltar a última nota, solicita reset das vozes e reinicializa contadores/fase. Ao desligar HOLD, `parameterChanged` reconstrói a sequência usando as teclas ainda pressionadas e solicita reset.

LOCK preserva ajustes ao mudar presets, conforme o [manual oficial, seção ARPEGGIATOR](https://tal-software.com/downloads/docs/TAL-U-NO-LX-UserManual.pdf). O binário contém `arpnotloadsettings`; o caminho completo de carregamento de presets ainda não foi decompilado. Essa conclusão é documentação do fabricante, corroborada por nome de parâmetro, e não recuperação integral da implementação de LOCK.

## Aplicação proposta ao KODA

O KODA atual não possui arpejador funcional. `Source/PluginEditor.cpp` desenha os controles com `dummy`; `Source/PluginProcessor.cpp::handleMidi` envia note-on/off diretamente a SynthDSP. O playhead fornece somente BPM ao synth. São observações do código local.

Proposta de projeto, não comportamento já implementado:

1. Criar um módulo de arpejo entre os eventos MIDI e SynthDSP, com estado pertencente à thread de áudio e buffers de capacidade definida, sem alocações durante processBlock.
2. Separar teclas pressionadas, sequência retida por HOLD e nota atualmente emitida. Preservar canal/velocidade e definir explicitamente a política de notas iguais em canais diferentes.
3. Implementar primeiro UP/DOWN/UP&DOWN, range 1–3 e curva livre recuperada; adicionar relógio por amostra e alinhamento PPQ para HOST.
4. Acrescentar MIDI clock e gatilhos do canal 16 com consumo explícito dos eventos de sincronização. Não confundir HOLD com CC64.
5. Adicionar parâmetros próprios ao APVTS, sem renumerar os 47 parâmetros existentes; ligar botões, seletores, LEDs e fader a esses parâmetros.
6. Tratar LOCK na aplicação de presets. Preservar os ajustes do arp quando bloqueado e definir separadamente a restauração de sessão da DAW.
7. Limpar notas emitidas e estado retido em panic, reset, mudança de sampleRate e transições de modo/fonte; testar notas presas, transporte e mudanças de bloco.

Não copiar o reset global do TAL cegamente: no KODA ele pode interromper sustain ou vozes cujo ciclo de vida pertence a outra entrada. O módulo deve encerrar as notas que ele próprio gerou.

## Cobertura e validação restante

| Questão | Estado |
| --- | --- |
| Ordenação, expansão e retorno UP&DOWN | Respondida por pseudocódigo e comparadores |
| Curva livre e seleção de divisões | Respondida estaticamente; discrepâncias de rótulos registradas |
| HOLD e liberação da última tecla | Respondida estaticamente |
| HOST, MIDI CLK, EXT16 | Caminhos principais recuperados; combinações e transporte parcialmente respondidos |
| LOCK | Semântica documentada; carregamento interno ainda parcial |
| Modos especiais | Branches recuperados; associação completa aos rótulos pendente |
| Gate, notas fora da faixa e equivalência sonora | Não estabelecidos |
| Integração no KODA | Proposta documentada; não implementada |

Antes de afirmar reprodução fiel, comparar TAL e KODA com acordes de uma, duas e três notas, inversões, três ranges, HOLD e troca de acorde, mudanças de preset com LOCK, várias taxas de amostragem/tamanhos de bloco, clock MIDI, canal 16 e transportes start/stop/seek. Nenhum teste sonoro ou de execução do plugin original foi realizado nesta análise estática.
