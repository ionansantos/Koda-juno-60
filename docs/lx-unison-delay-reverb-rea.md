# TAL-U-NO-LX 5.1.5 — UNISON, DELAY, REVERB e FX LOCK

Investigação de 19/10/2026, restrita à faixa indicada na imagem. O teclado e as demais funções não são objeto deste relatório. Não houve alteração do KODA, instalação de versões ou execução do TAL nesta etapa.

## Identidade, método e fontes

Referência instalada: `/Library/Audio/Plug-Ins/Components/TAL-U-NO-LX-V2.component/Contents/MacOS/TAL-U-NO-LX-V2`, versão declarada 5.1.5, SHA-256 `99bcc10a28984d52e46299f8369b7a3cb0648ae99358ec3e8d5628d5f113aba6`. A fatia ARM64 previamente derivada tem SHA-256 `f42ba995938a5df3a0beed6d28c214b80207c919f692306650222a32b1ef55e0`; ambas as identidades foram verificadas novamente.

Foi usado REA com Ghidra 12.1.4, pela rota local já funcional, sem alterar o registro global do MCP. A nova sessão produziu 25 registros de operações, incluindo abertura, buscas, decompilação, leitura de tabelas, assembly, exportação e fechamento. As operações concluíram sem erro. O banco nativo foi fechado; o bundle completo permanece salvo.

[Índice de Evidence IDs e resultados completos](../evidence/lx-effects/rea-20261019/INDEX.md). [Identidade e escopo](../evidence/lx-effects/rea-20261019/identity.json). [Verificação de que os fontes do KODA permaneceram iguais](../evidence/lx-effects/rea-20261019/verification.json).

Dois resultados anteriores, com a mesma identidade do alvo, foram reutilizados: `SynthEngine::parameterChanged`, Evidence `ev_088cae064f597d74469a9ff0010beef35d7d980fc1cd55a3a37cf985df88dc29`, e `TalCore::processBlock`, Evidence `ev_71e37182bed5a7abc82d87ad3081854b53faf5cccbcbefb7c91fb58f45379fa0`. Isso evita repetir a decompilação de código já recuperado.

**Observação** significa bytes, instruções ou operações presentes no resultado do REA. **Derivação** significa fórmula/interpretação calculada desses fatos. Não se afirma execução observada, código-fonte original ou equivalência sonora.

## Posição na cadeia

O Unison é produzido dentro de cada SynthVoice, antes dos filtros/envelopes dessa voz. Depois da soma das vozes e do processamento comum, a ordem observada é:

```mermaid
flowchart LR
    A[Oscilador central + Unison por voz] --> B[Filtros e envelope por voz]
    B --> C[Soma das vozes e processamento comum]
    C --> D[Chorus]
    D --> E[Delay estéreo]
    E --> F[Reverb estéreo com entrada mono]
    F --> G[Saída]
```

Portanto, o reverb recebe também as repetições do delay. Um Unison reconstruído apenas duplicando a saída final do instrumento não reproduziria essa organização.

## UNISON — VOICES, DET, WIDTH

Fontes principais: [SynthVoice::process, 0x4492c](../evidence/lx-effects/rea-20261019/030-voice-process.json), Evidence `ev_5c737013be92f29c616768f0fd9ad55d7869f9639299827e2ff505a22700ab02`; [Vco::process, 0x45744](../evidence/lx-effects/rea-20261019/033-vco-process.json); [formatação de VOICES, 0x66658](../evidence/lx-effects/rea-20261019/038-unison-voices-display.json).

Para valor normalizado `v`, o texto do controle é:

```
v <= 0,001: 1
caso contrário: floor(4*v + 3,5)
```

As indicações são **1, 3, 4, 5, 6, 7**; não há estado de duas camadas nesse caminho. O setter armazena a quantidade adicional `N = indicação - 1`, ou zero quando desligado. O consumidor confirma que o Vco central sempre existe e que N Vcos adicionais são processados **dentro da mesma SynthVoice**. Isso não equivale a selecionar a polifonia do instrumento.

O DET normalizado `d0` vira `d=d0²`. Os osciladores adicionais usam desvios:

```
+d, -d, +3d, -3d, +5d, -5d
```

A transformação de altura em Vco contém o fator `1/12` e a referência 69/440 Hz; a interpretação desses desvios em semitons é derivada desse consumidor. Por exemplo, DET=0,1 produz d=0,01 semitom, ou um cent por unidade básica. Não se trata de espalhamento aleatório aplicado ao áudio já somado.

Para WIDTH `w`, o trecho escalar recuperado produz:

```
L = 0,5 * [(1-w²)*central + Σpositivos (1-w)*osc + Σnegativos (1+0,5*w)*osc]
R = 0,5 * [(1-w²)*central + Σpositivos (1+0,5*w)*osc + Σnegativos (1-w)*osc]
```

Em w=0, as duas somas são iguais. Em w=1, o central sai dessa soma e os lados alternados ficam separados. Com quantidade ímpar de osciladores adicionais, pode haver um membro sem seu par oposto. A atenuação específica observada é ×0,5, independente de N; esse trecho não usa normalização 1/N ou 1/√N. Outros ganhos comuns e controles por voz continuam a atuar depois.

## DELAY — WET, TIME, SYNC, SPREAD, TONE, FDB

Fontes: [DelayEngine::parameterChanged, 0x50680](../evidence/lx-effects/rea-20261019/020-delay-parameters.json), Evidence `ev_fbf80dc766d1a5f6434e3d5317189f8d532fe9a134aeb0cfba2227141966be03`; [construtor de DelayFx, 0x49d94](../evidence/lx-effects/rea-20261019/024-delay-constructor.json); [processamento, 0x445f4](../evidence/lx-effects/rea-20261019/022-delay-process.json) e seu [assembly](../evidence/lx-effects/rea-20261019/031-delay-assembly.json).

Há um DelayFx e um buffer por canal. Cada buffer tem aproximadamente `4*Fs+1` samples. O valor interno TIME não é diretamente um número de segundos: ele seleciona uma fração desse buffer de quatro segundos.

### Tempo livre e sincronização

Com TIME normalizado `t`:

```
r = exp(2,9957323*t)*0,052105263 - 0,042105265
```

r varia aproximadamente de 0,01 a 1. Como o tap usa `floor((bufferLength-1)*r)`, o atraso físico livre é aproximadamente **40 ms a 4 s**, antes do SPREAD. Interpretar r como 10 ms a 1 s perderia o fator de tamanho do buffer.

A leitura do tap é inteira; a posição desejada recebe suavização de primeira ordem. O construtor configura coeficientes equivalentes a `(target + previous*c)/(c+1)`, com `c≈Fs*0,04535147`. A derivação é uma constante de tempo de aproximadamente 45 ms; não foi medida uma resposta de áudio nesta sessão.

SYNC usa `floor(20*t+0,5)` e 21 posições. O [helper 0x442c0](../evidence/lx-effects/rea-20261019/025-delay-sync.json), Evidence `ev_f5e1596214b44679dce6d54d73e64ae208619414cca43fa5e9cc13053fb5b926`, recupera estes rótulos em ordem:

```
1/16, 1/16T, 1/8., 1/8, 1/8T, 1/8.,
1/4, 1/4T, 1/4., 1/2, 1/2T, 1/2.,
1/1, 1/1T, 1/1., 2/1, 2/1T, 2/1.,
4/1, 4/1T, 4/1.
```

**Particularidade do binário:** o índice 2 grava coeficiente 5,625 e compartilha o texto `1/8.` com o índice 5, que grava 11,25. Musicalmente, o índice 2 corresponde à duração de 1/16 pontuada, mas não se deve substituir o rótulo observado por essa inferência.

Os coeficientes são `3,75; 2,5; 5,625; 7,5; 5; 11,25; 15; 10; 22,5; 30; 20; 45; 60; 40; 90; 120; 80; 180; 240; 160; 360`, com pequenas aproximações float nas tercinas. A fração é `coeficiente/BPM`; o tempo físico deriva de aproximadamente `4*coeficiente/BPM`. Por exemplo, 1/4 a 120 BPM resulta em cerca de 0,5 s. Cada canal é limitado à fração 0,01..1; divisões longas podem atingir o limite de quatro segundos.

### SPREAD e FDB

Para SPREAD `s`:

```
s < 0,5: mL=s+0,5; mR=1
s >= 0,5: mL=1; mR=1,5-s
rL=clamp(mL*r,0,01,1)
rR=clamp(mR*r,0,01,1)
```

O centro produz o mesmo atraso nos dois lados. O extremo reduz o tempo de um lado para a metade. Além disso, SPREAD modifica os coeficientes de realimentação. Para FDB normalizado `f`:

```
g = sqrt(0,99*f²) ≈ 0,9949874*f
feedbackL = g*(0,5*mR+0,5)
feedbackR = g*(0,5*mL+0,5)
```

Os canais têm processamento próprio; esses vínculos entre parâmetros não demonstram troca de áudio entre L e R.

### TONE, realimentação e mistura

Na metade inferior, TONE altera a configuração do estágio low-pass com curva `(1,7*p+0,15)²`, corrigida pela taxa de amostragem. Acima do centro, o low-pass permanece aberto e o coeficiente do high-pass varia com `(1,7*p-0,85)²`. Os polinômios completos estão preservados no setter; não foi estabelecida uma conversão calibrada desse knob para Hz.

A recorrência contém: entrada + tap realimentado → filtro low-pass → high-pass → saturação suave → escrita no buffer. Para a saída do high-pass h, a saturação observada pode ser expressa como:

```
u=2*h; a=abs(u); Q=a*(a+3)+6
buffer[write] = u*Q/(a*Q+12)
```

O **retorno** da função é o tap antigo, antes dessa escrita. O pseudocódigo isolado inferiu retorno void, mas o assembly mantém o tap em s0 até `ret`, e o caller o usa como float. É um caso concreto em que copiar apenas o pseudocódigo produziria uma implementação errada.

Para WET `x`, `dry=1` até x=0,5; depois `dry=2-2*x`. O wet interno é x. No caller:

```
out = dry*input + 5*x*DelayFx(0,2*input)
```

Não cancelar 0,2 e 5 através da recorrência não linear. WET=0 solicita limpeza dos buffers/estados; não é apenas redução silenciosa da mistura.

## REVERB — WET, SIZE, DELAY, TONE

Fontes: [ReverbEngine::parameterChanged, 0x510c4](../evidence/lx-effects/rea-20261019/021-reverb-parameters.json), Evidence `ev_9a22a6f7e67edfa212641f4ffa78ef5810b2f46174ebbef2eccf9224b6afdfa4`; [inicialização, 0x42558](../evidence/lx-effects/rea-20261019/023-reverb-init.json); [construção dos motores](../evidence/lx-effects/rea-20261019/035-engine-construction.json), junto do processBlock reutilizado.

**WET:** mesma curva dry por trechos do delay, mas o ganho wet é `0,5*x²`. Logo, em x=0,5 há dry=1 e wet=0,125; não é crossfade linear ou de potência constante. Em zero, o processamento é desviado e uma limpeza fica pendente para a próxima ativação.

**DELAY:** é o pré-delay da reverberação. O setter armazena p², e o consumidor escolhe um tap inteiro `floor((N-2)*p²+1)` de um buffer cujo tamanho é próximo de Fs, arredondado pelo inicializador. A derivação física é de aproximadamente um sample até um segundo. A entrada desse pré-delay é `(L+R)*0,25`, portanto a excitação é mono, embora a saída do reverb seja estéreo.

**SIZE:** o valor de controle vira:

```
sizeInternal = 1,0510527 - 0,051052634*exp(2,9957323*(1-p))
```

Ele varia aproximadamente de 0,03 até 1 e alimenta fatores internos de realimentação: `sizeInternal*0,99` e `sizeInternal*0,495`. Isso não estabelece um RT60 em segundos nem uma dimensão de sala em metros.

**TONE:** controla filtros high-pass e low-pass em série, com Q=0,68 e ganho=1. As tabelas de métodos foram lidas pelo REA para confirmar as classes concretas; não são nomes inferidos só pela posição dos coeficientes.

```
p <= 0,5: LP=(1,5*p+0,25)^4; HP=0
p > 0,5:  LP=1; HP=(1,5*p-0,75)^4
frequência solicitada de cada filtro = 10 + 20000*valor
```

Isso deriva aproximadamente LP=88 Hz/HP=10 Hz no mínimo, LP=20.010 Hz/HP=10 Hz no centro e LP=20.010 Hz/HP=6.338 Hz no máximo. São frequências solicitadas pelo consumidor, não uma resposta medida nem garantia de comportamento em todas as taxas de amostragem.

### Rede interna

A inicialização e o consumidor mostram seis ramos de atraso modulado por canal, somados com sinais alternados, seguidos de onze estágios all-pass por canal. As leituras dos ramos modulados têm interpolação linear. Os comprimentos de buffers são ajustados em função de Fs e passam por seleção de tamanhos no inicializador.

A construção fixa parâmetros internos não expostos nessa faixa: +0x18=0,4; +0x1c=0,1; +0x20=1. O primeiro controla períodos dos moduladores, com referência `((1-q)^2)*88200+1000` e fatores 1..1,0833334 nos seis ramos. **Esses valores não são os tempos de atraso de áudio da sala.** O segundo entra na profundidade de modulação; o terceiro escolhe a saída após os all-pass. Os buffers efetivos são construídos separadamente. Confundir esses proprietários levaria a um reverb estruturalmente diferente.

As constantes, seeds, tabelas de comprimentos e coeficientes permanecem no resultado completo. Não se identificou esta rede como uma biblioteca externa específica nem se demonstrou que um reverb genérico teria resposta equivalente.

## FX LOCK e carregamento de presets

O rastreamento passou por TalCore::setStateInformationFromXml até [PresetHelper::setXmlPrograms, 0x36864](../evidence/lx-effects/rea-20261019/032-preset-helper.json), Evidence `ev_213759c5c11ada636cd08df271c695b3fa07ea579af8b1fc55f593a891f4f0aa`.

O helper lê `paramFxLock`; somente quando seu valor atual é menor que 0,5 aplica os quatro parâmetros de reverb e os seis de delay vindos do preset. Os três parâmetros de Unison são carregados fora desse bloco. Portanto, **esse LOCK preserva delay e reverb na troca de preset; não protege o Unison**. O mecanismo opera sobre valores de parâmetros, não congelando a execução dos buffers de áudio.

Também há distinção entre default do parâmetro e fallback para campo ausente no XML: por exemplo, a configuração declarada de Delay Time é 0,5, mas o loader usa 0,3 quando o atributo não existe; para Feedback, o default declarado é 0 e o fallback do loader é 0,2. Ambos são fatos distintos e precisam ser preservados numa futura reconstrução de presets.

## Cobertura e limites

| Pergunta | Resultado |
| --- | --- |
| VOICES e quantidade real de camadas | Respondida pelo setter, consumidor e formatter |
| DET e WIDTH | Curvas e soma estéreo recuperadas estaticamente |
| Delay: tempo livre, sync, spread, feedback e wet | Respondida, incluindo buffer físico e limite |
| Delay: TONE e saturação | Recorrência recuperada; resposta de áudio não medida |
| Reverb: quatro controles e posição na cadeia | Respondida estaticamente |
| Reverb: rede e recursos | Topologia, constantes e inicialização recuperadas; portabilidade exata ainda não validada |
| LOCK e inclusão/exclusão de Unison | Respondida no carregador de presets |
| Correspondência sonora com outra implementação | Não testada nesta etapa |

O código compilado contém processamento vetorial e retornos de ponto flutuante que o decompilador representa imperfeitamente. O caso do retorno de DelayFx já foi resolvido por assembly; algumas expressões de filtros e conversão rápida de pitch ainda exigem essa conferência antes de uma tradução literal.

Para implementar depois no KODA, a base correta é separar: Unison por voz; DelayEngine dono dos dois buffers e filtros; ReverbEngine dono da rede/pré-delay; e política de LOCK no carregador de presets. A validação seguinte deve usar impulsos e notas controladas no TAL real para medir tempos, caudas, banda, ganho e imagem estéreo. Esta investigação entrega o mapa e as evidências; não entrega uma integração ou uma alegação de som idêntico.
