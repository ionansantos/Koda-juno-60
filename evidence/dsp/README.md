# DSP: evidência e limites da reimplementação

Alvo: Mach-O i386 extraído do VST, SHA-256 `b46668c96111476a3bde38e3367e8ca323178070ced25a51f47f9dd51dc65359`. As observações abaixo são estáticas. Não houve comparação de áudio com o plugin executando em host Intel de 32 bits. Portanto, equivalência sonora/sample a sample **não foi demonstrada**.

Cada `*.json` de REA conserva Evidence ID, alvo, perfil, limitações e pseudocódigo ou assembly completo. Os `*.txt` correspondentes ajudam a leitura; não são código-fonte original. `constants.json` resolve dados relativos ao PC da instrução `CALL ___i686_get_pc_thunk_bx`; `file_offset` e VA coincidem nas regiões em questão. Os campos double só são significativos quando o assembly lê float64. Máscaras de sinal não são números comuns. `otool-disassembly.txt` é corroboracão local adicional e contém trechos lineares que interpretam jump tables como instruções: não confundir esses trechos com blocos executáveis.

## Ordem efetivamente observada

`SynthVoice::process`, **0x258ce**: DCO saw/pulse/sub + noise → Filter3 (quando p38=0) → VCA, duas amostras intermediárias por chamada. `SynthEngine::process`, **0x1f16**: seis vozes somadas → possível filtro compartilhado (p38!=0) → FIR de decimação → ganho → **três HPFs em série** → chorus estéreo. O HPF do código está depois do LPF/VCA/soma, e não antes do LPF. Preservar a ordem solicitada conceitualmente pelo usuário alteraria o comportamento observado. [Pseudocódigo engine](engine_process.json), [voz](voice_process.json).

`SynthEngine::SynthEngine`, **0x327c**, aloca seis `SynthVoice`; `setNoteOn`, **0x1c00**, reusa a voz cujo número MIDI coincide, senão procura voz sem gate circularmente e rouba após seis tentativas. `setNoteOff`, **0x1ba0**, desliga gate de todas as vozes desse número. Não há estado de canal no corpo observado; o argumento adicional é ignorado no pseudocódigo. [Construtor](engine_ctor.json), [note on](note_on.json), [note off](note_off.json).

## DCO, PWM, sub e afinação

`RecOsc::getNextSample`, **0x24b1c**, e o saw inline da voz usam correção de degrau por tabela: 66 amostras de buffer circular, stride **32**, consulta `int((i+overshoot)*32 + 0.5)`, tabela em **0x30840**. 2048 valores foram extraídos para [blep_table.json](blep_table.json) e `Source/DSP/BlepTable.h`. Não é simplesmente um oscilador com polyBLEP inventado. [Oscilador](osc.json), [assembly](osc_assembly.json), [construtor da voz](voice_ctor.json).

O acumulador de fase incrementa por `frequency/sampleRate`. Cada oscilador é chamado **duas vezes** por amostra entregue ao host; o sample rate armazenado não é dobrado no construtor. A fórmula interna de frequência usa `440 * 2^((pitch - 36 - 45)/12)` (constantes float64 440 em **0x30478**, float32 36 em **0x303d8**, 45 em **0x303dc**, 12 em **0x3039c**) e limita a **19000 Hz**, **0x303e0**. Isso exige levar em conta as duas atualizações ao interpretar a afinação audível.

`pitch = note + 2*(p2-0.5) + bend*p31*12`; p33<0.25 adiciona -12, p33>0.5 adiciona +12, resto zero. LFO de pitch acrescenta `lfo*p10^3*12`. Sub usa note-12 e ganho **0.9*p5**. Pulse compara fase secundária com `width*0.8+0.1`; as duas constantes são **0x30354/0x30358**. Sem PWM, width passado = **0.48** (`0x3ef5c28f`). p36!=0 usa `.5+.5*p9`; caso contrário p8!=0 usa envelope*p9; caso contrário seno LFO*.5*p9+.5. [Voz](voice_process.json).

## Envelope

Construtor **0x255b8** define escala `1000/sampleRate * 0.15 = 150/sampleRate`. Há três estados de envelope por voz: amplitude, filtro e gate. O filtro tem flag de ataque instantâneo específica. Ataque: `a=(1-A)*0.7+0.22`, `level += a^4 * scale*(level+0.2)`. A=0 recebe incremento extra **0.02** e tratamento instantâneo. Decay: `level -= (1-D)^4*scale*(level^4+0.2)`. Release: `level -= (1-R)^4*scale*(level+0.05)`. Piso **0.005**. O envelope de filtro tem condições diferentes para release zero e ataque instantâneo; o pseudocódigo integral é a referência. Os coeficientes estão em **0x303bc..0x303d4**. [Voz](voice_process.json), [construtor](voice_ctor.json).

Amount de filtro é `p15+p29*velocity`; envelope contribui com o **cubo de `±envelope*amount²`**. A polaridade é p14. O LFO contribui `.5*p16³*lfo`. Keytrack p17 usa log, fator **17.312339782714844** em **0x30480**, offset 24 e fórmula de pitch; a mistura ainda multiplica o resultado por p17. O drift de cutoff é `(rand/RAND_MAX - .5 + 29*old)/30`, depois acrescenta `cutoff^4*drift`.

## LPF Filter3

Endereço **0x24d3e**. O pseudocódigo não permite justificar trocar o filtro por um SVF ou ladder padrão. [Filtro](filter.json), [assembly](filter_assembly.json).

```
x = cutoff * min(1, 44100/sampleRate) * 0.5
correction = 1.873*x³ + 0.4955*x² - 0.649*x + 0.9988
resonanceCorrection = -3.5364*x² + 1.8409*x + 0.9968
z = -PI*correction*x
coefficient = 40000 * (1 - (1+z+z²/2+z³/6+z⁴/24+z⁵/120))
```

Cada chamada atualiza **oito** estados consecutivos. Não confundir automaticamente oito estados com uma declaração física de filtro analógico de oito polos: o filtro é atualizado duas vezes por saída e ainda há atrasos/realimentação. A não linearidade observada, com `u = input * 5.749999763793312e-5`, é `u*q/(q*abs(u)+12)`, `q=(abs(u)+3)*abs(u)+6`. Realimentação `-2.6*resonance*(.8+.2*random)*previousOutput*resonanceCorrection`, com taper a partir de x=.4. Ganho de saída **.35**; duas transformações cúbicas assimétricas com ganho **2.5**, coeficientes positivos/negativos diferentes: -1/6, -0.16686666011810303, -0.1466662734746933, -0.15666626393795013. Dados **0x30360..0x303b4**. O código gerado mantém as constantes e topologia; a ordem final de arredondamento não foi validada.

## FIR, ganho e HPF

Construtor **0x327c** inicializa coeficientes FIR `.5, .3077392578125, -.07794189453125, .02618408203125, -.007080078125, .0010986328125`; estrutura de atualização de nove estados em **0x1f16**. Ganho global `p1²*p19`. Cada um dos três HPFs faz `y = x - previousX + (0.999 - 0.4*p11³)*previousY`. Há ruído anti-denormal de **2.3283064365386963e-10**. Estes coeficientes não são recalculados por sample rate no trecho observado. [Engine](engine_process.json), [construtor](engine_ctor.json).

## LFO e tempo

`Lfo::setRate` **0x14e8**: incremento da posição de tabela `256*Hz/sampleRate`. `tick` **0x150c** interpola tabelas de 257 pontos. Engine livre usa **20*p24⁴** Hz. O delay p25 espera p25*sampleRate amostras, depois aumenta fade por **5e-10*sampleRate** a cada tick.

Ghidra não recuperou automaticamente os destinos de `setWaveform` **0x170b**. A tabela raw em **0x1710** foi decodificada, corroborada com instruções: slot0 → **0x1724**, triangle iniciando em 0; slot1 → **0x18fc**, seno com passo 1/256 e 2PI; slot2 → **0x18ac**, saw; slot3 → **0x1873**, square; slot4 → **0x179c**, tabela exponencial que `tick(4)` não usa, pois este seleciona sample/hold aleatório. `tick(>=5)` usa random atrasado. [Bytes REA](lfo_branches_raw.json), [tick](lfo_tick.json), [rate](lfo_rate.json).

A tabela de sync de noteOn está em **0x1cf4**, 13 entradas relativas ao PC **0x1c0e**. Slots1..12 multiplicam BPM/60 por **5,2,1,.5,.25,.125,7.5,3,1.5,.75,.375,.1875**. O primeiro é **5**, não 4: leitura de **0x30344**, apesar do rótulo 1/16. Conservamos a peculiaridade. O original atualiza taxa sincronizada no noteOn; o projeto conserva esse latch. [Bytes REA](sync_raw.json), [assembly](note_on_assembly.json), [corroboração otool](otool-disassembly.txt).

## Chorus

Quatro linhas de delay no construtor **0x327c**, p34 seleciona duas, p35 seleciona as outras. Base **7ms**, rampas contrárias; taxa nominal .5/.83Hz (incremento de rampa **2/sampleRate** ou **3.32/sampleRate**), delay `(ramp*.3+.4)*7*sampleRate*.001`. Capacidade `floor(sampleRate*.007)+2001`. Interpolação é recorrente/allpass: `(1-fraction)*sampleA+sampleB-(1-fraction)*previousOutput`. LP suave .24872541427612305/.751274585723877. Wet passa por HPF .995 e ganha **1.4**. [Engine](engine_process.json), [construtor](engine_ctor.json).

## Lacunas explícitas

O motor é uma reconstrução executável, com `// TODO verificar` nos desvios. Ainda falta validação diferencial de áudio; libc rand é reutilizado, mas seed/estado global dependem do host e da libc; o índice de tabela 256 recebe proteção de limite; não há preservação de estado de canal; tails de filtro e envelope do filtro compartilhado exigem refinamento; proteção para estado não finito é escolha da implementação. Alguns estados transitórios do filtro e alocação/reuso de voz não têm igualdade bit a bit demonstrada.

Não foi encontrado arpeggiator em símbolos ou na estrutura/processamento de SynthEngine analisados. Isso é uma busca negativa delimitada, não prova universal de ausência. O projeto não adiciona um arpeggiator apresentado falsamente como comportamento recuperado.

## Verificação executada

`tests/dsp_test.cpp` cobre 22050/44100/48000/96000 Hz, silêncio inicial, note on, release, reset, chorus estéreo, os **120** programas recuperados, polyphony/roubo, pitchbend, extremos e saída finita. Compilado C++17 com clang, AddressSanitizer e UndefinedBehaviorSanitizer; todos passaram. Isso comprova estabilidade dos cenários testados, **não** igualdade com o TAL original.
