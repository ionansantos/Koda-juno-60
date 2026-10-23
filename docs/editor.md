# Editor original e reconstrução

O editor é a classe **TalVstEditor**, distinta de TalVst e ToguSynthProgram. Sua função `open` 0x7978 cria um CFrame e38 controles; evidência ev_0bee1859808a641ad34db0d1cd319a0e0288c623b293958e9155fa88485569d8. O construtor0x6d04 deriva o tamanho do bitmap128; ev_8bc0121b3d146bf9977c91f954a0e1e6d6c67973b84e3de84588d3a102bb79d8. O recurso instalado mede **695x350 pixels** (leitura do PNG extraído, corroborando a imagem). O clone desenha seu próprio painel JUCE na mesma geometria principal, com uma área inferior para os9 parâmetros invisíveis no original e seletor de presets acrescentado no topo.

| Tipo | Quantidade | Helper / endereço |
|---|---|---|
| Knob animado | 2 | setKnob0x7054 |
| Slider vertical | 20 | setVerticalSlider0x7142 |
| Botão metal | 5 | setMetalOnOffButton0x726a |
| Botão colorido | 6 | setColorOnOffButton0x7336 |
| Botão MIDI learn | 1 | setMidiLearnOnOffButton0x74c2 |
| Botão global MIDI learn | 1 | setGMidiLearnOnOffButton0x758e |
| Switch horizontal | 1 | setHorizontalSwitch3 0x6f46 |
| Menu | 2 | setOptionMenu0x7664 |

Confira os totais do JSON de layout: [editor-layout.json](../evidence/parameters/editor-layout.json), com índice e x/y para todos os38 controles. O switch chamado Switch3 na symbol table passa4 estados e divide a altura da imagem por4; evidência ev_957437a466a5b622502b1849dac8b34cc688fd34757e5ea7ccef3d28f080acde. Portanto o nome do helper não garante três posições.

Os sliders têm curso y+1..y+69 e mínimo0/máximo1; ev_25d412dd8e6796400ad5068895fc26e9a4af394ed98d4cbe2be16c37e277df01. Os knobs têm33 pixels de altura e51 frames de animação; ev_1134ccf97f31c175e2d1ff9bf673314459581fa940e1ba43e47f91ee3b12e0c3. As dimensões horizontais dependem das imagens; o clone mantém33x33 e20x76 como escolhas gráficas e marca TODO para verificar hitboxes.

Fluxo original0x6c96: lê tag do CControl, rejeita tag>46, lê o valor float e chama setParameterAutomated. Evidência ev_71e17a04c42f7154186f410992967e3dbb2691d5837e9f7363e28d40f7f32365. No clone isso é reproduzido com APVTS SliderAttachment/ButtonAttachment/ComboBoxAttachment. O seletor de presets chama getProgramName/setCurrentProgram e acompanha mudanças externas pelo timer.

O desenho mantém os grupos **CONTROL / CHORUS / LFO / DCO** na linha superior e **HPF / VCF / VCA / ENV** na inferior, com DCO/VCF pitch wheel e master tune/volume no canto inferior esquerdo. A arte original não foi incorporada no código; os painéis são desenhados em JUCE. Esta reconstrução confirma layout e tags; não confirma pixel-identidade nem comportamento de MIDI learn.

```cpp
// 0x7978 exemplos de posições observadas
setKnob(frame,173,258,1); // VOLUME
setKnob(frame,128,258,2); // MASTERPITCH
setVerticalSlider(frame,284,248,12); // CUTOFF
setVerticalSlider(frame,530,248,20); // ATTACK
setOptionMenu(300,169,27,13,27); // waveform
setOptionMenu(329,169,27,13,28); // clock
// TODO verificar: hitboxes exatos, MIDI learn, aliases e aparência no runtime original.
```

