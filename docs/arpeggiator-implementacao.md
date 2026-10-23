# KODA-JUNO-60 0.3.0 — arpejador implementado

Em 18/10/2026 foi implementado o módulo de arpejo e conectado o painel. AU e VST3 arm64 foram compilados, assinados localmente e instalados na biblioteca do usuário. O aplicativo standalone está em `../dist/arpeggiator-0.3.0/`. A versão anterior do AU foi preservada em `../dist/installed-before-arpeggiator-20261018-163941/`. A cópia inicial dos fontes fica em `../dist/before-arpeggiator-20261018-162747/`.

## Controles

- **ON:** ativa o arpejador; desligado, o instrumento recebe MIDI diretamente.
- **MODE:** DOWN, UP&DOWN e UP; **RANGE:** uma, duas ou três oitavas.
- **TEMPO:** curva livre de 1–25 Hz; com SYNC, escolhe uma das 18 divisões recuperadas. O valor é mostrado no painel.
- **HOLD:** mantém a sequência após soltar as teclas. Um acorde novo substitui o padrão quando todas as teclas anteriores foram liberadas. Desligar HOLD libera a nota emitida e reconstrói a sequência usando as teclas ainda pressionadas.
- **LOCK:** preserva os ajustes ao trocar presets. A restauração de uma sessão da DAW restaura o estado completo, mesmo com LOCK ativado.
- **SYNC/HOST:** segue BPM e posição PPQ da DAW. O primeiro toque dispara imediatamente; depois segue a grade. Ao parar o transporte, libera a nota gerada.
- **SYNC/MIDI CLK:** conta F8; Start reinicia, Continue retoma e Stop libera a nota.
- **SYNC/EXT16:** note-on no canal 16 dispara um passo. As notas desse canal servem como gatilho e não entram no acorde.
- **NORMAL:** padrão normal; os outros modos são AS PLAYED, RANDOM e UP/DOWN REPEAT. Esses nomes descrevem os branches recuperados; não foi confirmada a correspondência literal com todos os rótulos do menu TAL.

Se mais de uma fonte estiver ativa, a prioridade explícita é EXT16, MIDI CLK, HOST. Sem posição de host válida, o relógio sincronizado usa o BPM disponível, ou 120 BPM. A prioridade combinada e o comportamento fora de um host são decisões KODA, não uma equivalência demonstrada com o TAL.

Os 47 IDs originais foram preservados; os 11 parâmetros novos foram acrescentados. Cada preset guarda seus ajustes de arpejo, com LOCK global. Sessões anteriores sem esses parâmetros carregam o arpejador desligado. As notas retidas em execução não são serializadas.

## Implementação e propriedade

`Source/Arpeggiator.h` é um agendador independente de JUCE, com buffers de capacidade fixa para todas as combinações válidas de altura/canal MIDI e sua expansão. Não aloca no processamento. A curva, a divisão e os contadores são preparados fora do avanço por amostra; não há exponencial por amostra.

O AudioProcessor encaminha eventos ao módulo e suas notas geradas ao SynthDSP, preservando a posição de sample dos eventos. A liberação das notas geradas ignora o sustain físico para evitar notas presas. HOLD e sustain têm estados distintos. Reset, panic, mudanças de roteamento e mensagens de canal limpam os recursos correspondentes.

Políticas explícitas de segurança KODA: alturas expandidas fora de 0..127 são omitidas; notas iguais em canais distintos mantêm identidades distintas; o arpejador encerra suas próprias notas sem copiar o reset global do TAL em cada passo. Esses pontos podem diferir do original.

O build agora reúne os testes no alvo CMake `tal_tests`, evitando a divergência anterior entre o conjunto compilado pelo script e o conjunto registrado no CTest.

## Verificação executada

Os **seis testes CTest passaram**, incluindo os quatro anteriores e os novos testes de agendamento e integração com o AudioProcessor. A suíte cobre direções, oitavas, modos especiais, HOLD, sustain, identidade por canal, notas geradas, panic, program/LOCK, estado legado, clocks, PPQ e a ligação do botão HOLD visível ao parâmetro. O núcleo também passou com AddressSanitizer e UndefinedBehaviorSanitizer. O editor real foi renderizado e inspecionado visualmente em `../evidence/arpeggiator/runtime/koda-panel-final.png`.

Foi executada comparação **com o Audio Unit real do TAL-U-NO-LX 5.1.5 instalado**, usando `tools/au-arp-probe.cpp`, sem dispositivo de áudio ou GUI. Cada caso usa acorde C–E–G, saw sem sub/chorus, transposição explicitamente alinhada, bloco de 256 samples, 256 samples de silêncio inicial e seis segundos de renderização; as teclas são liberadas em quatro segundos. `tools/compare-au-arpeggiators.py` mede alturas por FFT no centro dos passos, sequência de alturas estáveis, intervalo mediano e presença/ausência de áudio após a liberação. Não compara formas de onda.

| Escopo | Resultado |
| --- | --- |
| 44,1 kHz: três direções × três ranges, HOLD, HOST, MIDI CLOCK e EXT16 | 13/13 casos coincidiram em alturas amostradas, ordem, intervalo mediano e comportamento após soltar teclas |
| 48 kHz: ordem das notas e HOLD/liberação | Coincidiram nos 13 casos |
| 48 kHz: HOST, MIDI CLOCK e EXT16 em 1/16 e 120 BPM | Coincidiram em notas e intervalo de aproximadamente 125 ms |
| 48 kHz: tempo livre, rate=0,5 | Divergiu: TAL aproximadamente 170 ms; KODA aproximadamente 185 ms entre passos |
| AU instalado | Passou em `auval -v aumu TuCl InAu` |
| VST3 e standalone | Compilação e assinatura verificadas; não foram testados em DAW/dispositivo de áudio real |

A causa do desvio do TAL no relógio livre a 48 kHz permanece desconhecida. O valor observado é compatível com uma razão 44,1/48, mas isso é uma hipótese sobre o plugin/host de teste. O KODA mantém a duração da fórmula independente da taxa de amostragem; não foi introduzida uma correção baseada nessa hipótese.

Não está demonstrada equivalência de timbre, envelope ou gate. Não foram comparadas todas as 18 divisões, todas as combinações de fontes, todos os modos especiais, mudanças de transporte arbitrárias nem todos os presets do TAL. LOCK foi testado no KODA, mas a carga de presets do TAL não foi comparada em execução. Não houve teste desta versão na interface do Logic.

Registros reproduzíveis em `../evidence/arpeggiator/runtime/`: `final-ctest.log`, `installed-au-validation.log`, `installation.json`, e `final-44100/results.json` / `final-48000/results.json`. Os JSON guardam caminhos, SHA-256 dos binários, parâmetros, SHA-256 dos renders e medidas por caso. Os registros REA da análise permanecem separados em `../evidence/arpeggiator/INDEX.md`.

Uma execução da matriz inteira de 13 casos levou aproximadamente cinco segundos de parede e 4,5 segundos de CPU, segundo `/usr/bin/time -l`. Os logs `final-44100-time.log` e `final-48000-time.log` preservam os números exatos. O máximo RSS reportado é por processo, não pico agregado da família. Não há baseline anterior equivalente, medição de tokens ou classificação cold/warm; esses valores não demonstram ganho de desempenho.

## Uso

Reabra a DAW para carregar a versão instalada. Ative ON e toque um acorde. Para manter o arpejo ao mudar de timbre, ative LOCK. Para acompanhar a DAW, ative SYNC e HOST e inicie o transporte. No standalone, use o modo livre; a ausência de posição de host não equivale a uma DAW tocando.
