# Parâmetros: evidência e limites

Foram encontrados **47 slots float**. O accessor `getParameter` em 0x520a compara o índice com 0x2f; `setParameter` em 0x51ae escreve o valor sem clamp no programa e nos Params do motor. Evidências: ev_8f7d768ef0154bcd025317c584ab404b5b3c7d523a63729408fec2e8d3edffd3; ev_d2aaa115c53c172bcbfac6cd87eb63097ac1ec8c6420498a91fd76b1941c7577.

A função `getParameterName` em 0x4e0c escreve primeiro **Param** e só despacha índices 0..35. O Ghidra não recuperou o jump table; reconstruí a tabela dos bytes preservados em [parameter_name_raw.json](../evidence/parameters/parameter_name_raw.json), com base PIC 0x4e14 e tabela em 0x4e3c. Evidência: ev_6900cb6b654caa0cfe22bfc1ef8f049612dd925fdb6ca4f792ab44323c693c8b; assembly ev_a67eda759336433f4eea395136470f2b9366bd1c04d8d495ecd400d94aa709d1. Os casos gravam letras em MOV imediatos; por exemplo p01 inicia em 0x4edb, p12 em 0x4fba e p35 em 0x4ecc. **p00, p07 e p36..p46 retornam literalmente Param**; os nomes amigáveis nesses slots são aliases da reconstrução, nunca nomes recuperados do host.

`getParameterLabel` em 0x4e00 e `getParameterDisplay` em 0x4e06 são apenas retornos. Portanto não há unidade ou range físico publicado nessas callbacks. Evidências: ev_2ad37640b30b49278d9dd1cb19a5cb3d9300aa26092fcb297d7a9f38771013e1; ev_e1fc319ceb81883d082214c07b2571d67588a67b70d0af4812cc54ab208f584f. Unidades Hz/ms/dB não foram inventadas no APVTS; confira as fórmulas verificadas no relatório DSP.

Ranges da reconstrução: sliders originais configuram explicitamente mínimo0 e máximo1 em 0x7142 (ev_25d412dd8e6796400ad5068895fc26e9a4af394ed98d4cbe2be16c37e277df01). Os menus 27/28 utilizam índice cru (0..4/0..12); `setOptionMenu` em 0x7664 converte getParameter para inteiro (ev_4614a69bc3a68ebbb67b4ba91568916c580ad3e7b6b95a415354163e44a9e642). As strings de menu em 0x24678 confirmam as cinco ondas e treze divisões: ev_7cb974c17fb2718e5402a9ed57df556dcb237c538defe6697c7c3bb948a19b3a. Campos ocultos 0,30,39..45 usam 0..1 no clone por inferência; o setter original não impõe limite. Os botões também conservam float0..1 para preservar bancos com valores intermediários.

Defaults abaixo são do **construtor genérico ToguSynthProgram** em 0x4ae0, não do primeiro preset de fábrica carregado no startup. O construtor zera47 floats a partir de objeto+0x18 e escreve dez defaults nãozero; ev_fa87530a18d74aff718b98d66f300e68f41ea0b4ebe7e8f92109f705831b3ad5. O startup do clone aplica o primeiro programa da fábrica, conforme o processador original. JSON/CSV integrais incluem nomes originais, aliases e ranges usados.

| Índice / ID | Nome original | Nome no clone | Range bruto do clone | Default genérico | Endereço do caso |
|---|---|---|---|---|---|
| 0 / p00 | Param | Reserved 00 | 0..1 | 0 | 0x4ed9 |
| 1 / p01 | VOLUME | VOLUME | 0..1 | 0.5 | 0x4edb |
| 2 / p02 | MASTERPITCH | MASTERPITCH | 0..1 | 0.5 | 0x4eed |
| 3 / p03 | SAW | SAW | 0..1 | 1 | 0x4f03 |
| 4 / p04 | SUBOSC | SUBOSC | 0..1 | 1 | 0x4f0b |
| 5 / p05 | SUBOSCVOLUME | SUBOSCVOLUME | 0..1 | 0 | 0x4f1d |
| 6 / p06 | NOISEVOLUME | NOISEVOLUME | 0..1 | 0 | 0x4f37 |
| 7 / p07 | Param | Pulse | 0..1 | 0 | 0x4ed9 |
| 8 / p08 | SQUAREPWMODE | SQUAREPWMODE | 0..1 | 0 | 0x4f4d |
| 9 / p09 | SQUAREPWINTENSITY | SQUAREPWINTENSITY | 0..1 | 0 | 0x4f67 |
| 10 / p10 | OSCLFOINTENSITY | OSCLFOINTENSITY | 0..1 | 0 | 0x4f8a |
| 11 / p11 | HPCUTOFF | HPCUTOFF | 0..1 | 0 | 0x4fa7 |
| 12 / p12 | CUTOFF | CUTOFF | 0..1 | 1 | 0x4fba |
| 13 / p13 | RESONANCE | RESONANCE | 0..1 | 0 | 0x4fcc |
| 14 / p14 | FILTERCONTOURBIAS | FILTERCONTOURBIAS | 0..1 | 0 | 0x4fe1 |
| 15 / p15 | FILTERCONTOUR | FILTERCONTOUR | 0..1 | 0 | 0x5004 |
| 16 / p16 | FILTERLFO | FILTERLFO | 0..1 | 0 | 0x5020 |
| 17 / p17 | KEYFOLLOW | KEYFOLLOW | 0..1 | 0 | 0x5035 |
| 18 / p18 | ENVGATE | ENVGATE | 0..1 | 0 | 0x504a |
| 19 / p19 | ENVVOLUME | ENVVOLUME | 0..1 | 0.5 | 0x5059 |
| 20 / p20 | AMPATTACK | AMPATTACK | 0..1 | 0 | 0x506e |
| 21 / p21 | AMPDECAY | AMPDECAY | 0..1 | 1 | 0x5083 |
| 22 / p22 | AMPSUSTAIN | AMPSUSTAIN | 0..1 | 1 | 0x5096 |
| 23 / p23 | AMPRELEASE | AMPRELEASE | 0..1 | 0 | 0x50af |
| 24 / p24 | LFO1RATE | LFO1RATE | 0..1 | 0 | 0x50c8 |
| 25 / p25 | LFO1DELAY | LFO1DELAY | 0..1 | 0 | 0x50db |
| 26 / p26 | LFO1TRIGGER | LFO1TRIGGER | 0..1 | 0 | 0x50f0 |
| 27 / p27 | LFO1WAVEFORM | LFO1WAVEFORM | 0..4 | 0 | 0x5106 |
| 28 / p28 | LFO1CLOCK | LFO1CLOCK | 0..12 | 0 | 0x5120 |
| 29 / p29 | ENVVELOCITY | ENVVELOCITY | 0..1 | 0 | 0x5135 |
| 30 / p30 | CUTOFFSMOOTH | CUTOFFSMOOTH | 0..1 | 1 | 0x514b |
| 31 / p31 | MODDCO | MODDCO | 0..1 | 0 | 0x5165 |
| 32 / p32 | MODVCF | MODVCF | 0..1 | 0 | 0x5177 |
| 33 / p33 | TRANSPOSE | TRANSPOSE | 0..1 | 0.5 | 0x5189 |
| 34 / p34 | CHORUS1 | CHORUS1 | 0..1 | 0 | 0x519e |
| 35 / p35 | CHORUS2 | CHORUS2 | 0..1 | 0 | 0x4ecc |
| 36 / p36 | Param | PWM manual mode | 0..1 | 0 | 0x4e26/0x4ed9 |
| 37 / p37 | Param | Global MIDI learn | 0..1 | 0 | 0x4e26/0x4ed9 |
| 38 / p38 | Param | Save CPU | 0..1 | 0 | 0x4e26/0x4ed9 |
| 39 / p39 | Param | Reserved 39 | 0..1 | 0 | 0x4e26/0x4ed9 |
| 40 / p40 | Param | Reserved 40 | 0..1 | 0 | 0x4e26/0x4ed9 |
| 41 / p41 | Param | Reserved 41 | 0..1 | 0 | 0x4e26/0x4ed9 |
| 42 / p42 | Param | Reserved 42 | 0..1 | 0 | 0x4e26/0x4ed9 |
| 43 / p43 | Param | Reserved 43 | 0..1 | 0 | 0x4e26/0x4ed9 |
| 44 / p44 | Param | Reserved 44 | 0..1 | 0 | 0x4e26/0x4ed9 |
| 45 / p45 | Param | Reserved 45 | 0..1 | 0 | 0x4e26/0x4ed9 |
| 46 / p46 | Param | MIDI learn | 0..1 | 0 | 0x4e26/0x4ed9 |

## Pseudocódigo relevante

```cpp
// 0x520a getParameter
if (index < 0x2f) return params[index];
// 0x51ae setParameter
if (index < 0x2f) {
    programs[currentProgram * 0x4d0 + 0x18 + index * 4] = value;
    params[index] = value;
    editor->setParameter(index, value);
}
// 0x4ae0 constructor; index = objectWordOffset - 6
for (int i = 0; i < 47; ++i) params[i] = 0;
params[1] = params[2] = params[19] = params[33] = 0.5f;
params[3] = params[4] = params[12] = params[21] = params[22] = params[30] = 1.0f;
// TODO verificar: significado dos slots 0 e 39..45 e unidades físicas ainda desconhecidas.
```

A análise foi estática da fatia i386 SHA256 b46668c96111476a3bde38e3367e8ca323178070ced25a51f47f9dd51dc65359. Não houve execução do VST nem comparação de automação em host32bit. Os JSONs de REA preservam limitações, pseudocódigo e assembly completos retornados pelo provider.


O comportamento DSP esclarece o alias p36: valor nãozero seleciona PWM manual; p38 seleciona a topologia de filtro para economizar CPU. Confira o relatório DSP e suas evidências engine0x1f16 / voice0x258ce. Atenção: a lista visual de ondas começa sin,tri; a ordem dos branches DSP é documentada separadamente e deve preservar os índices originais.
