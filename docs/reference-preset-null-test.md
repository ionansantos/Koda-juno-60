# Preset de referência no slot 72 e null-test

## Escopo e origem

O slot **72** (menu: `73 Arp Magic Synth`) recebeu uma recriação baseada no preset
**Dream Arp** do TAL-U-NO-LX **5.1.5**. O arquivo de origem é:

`~/Library/Application Support/ToguAudioLine/TAL-U-No-LX/presets/TAL Presets Bank/Arp/Dream Arp.pjunoxl`

O nome “Arp Magic Synth” é o rótulo solicitado para o slot. **Não** é o preset
original “Arp Magic Synth”, que não existe no banco instalado. Os 74 campos do
XML foram preservados em `evidence/reference-preset/source-fields.json`.

## Mapeamento aplicado

| Bloco | Valores |
| --- | --- |
| DCO | saw=1, sub=1, sub vol=.096, noise=.02, pulse=0, PWM mode=1, PWM=.704 |
| Filtro | cutoff=.272, res=.62, env mode positivo, env=.364, mod=.388, key=.416 |
| VCA/ADSR | gate, A=0, D=.396, S=.524, R=.688, volume=.388 |
| LFO | rate=.092, delay=0, trigger=1, waveform=0 |
| Chorus | I ligado, II desligado |
| Arp | on=1, sync=1, mode=UP, range=3 oitavas, rate=.352941, hold=0 |
| FX | delay wet=0, reverb wet=0; delay time=.3, feedback=.2; reverb size=.8 |
| Voice Tuning | 6 vozes do XML, sem alteração de pan |

Decisões confirmadas pela decompilação REA de `SynthEngine::parameterChanged`
(`0x53238`) e `SynthVoice::setNoteOn`/`process`:

- `vcamode=1` seleciona **GATE** no TAL; o KODA usa `p18=1` para o mesmo caminho.
- `filterenvelopemode=1` seleciona envelope **positivo**; o KODA usa `p14=0`.
- O TAL usa o cutoff normalizado diretamente. O motor U-No-60 do KODA usa
  `cutoff³`; por isso o slot 72 ativa `SynthDSP::setReferenceMode(true)` e usa o
  cutoff direto. Os demais programas continuam com a curva original.
- `maxpoly=.1818` não foi aplicado como limite de vozes nesta rodada; o valor
  não tem parâmetro equivalente no KODA e a fórmula do TAL ainda não foi
  reconciliada com o número de vozes observado.

## Implementação

- `Source/ReferencePreset.h`: valores imutáveis do slot 72 (47 parâmetros, 11
  de arp, 55 extensões, incluindo 36 de Voice Tuning).
- `Source/PluginProcessor.cpp`: aplica o preset ao programa 72, renomeia o slot,
  usa a tabela no `CLEAR` e ativa o modo de cutoff do slot 72.
- `Source/DSP/SynthDSP.*`: `setReferenceMode()` escolhe cutoff direto em vez de
  `cutoff³`.
- `tests/reference_preset_test.cpp`: nome, visibilidade, valores exatos, ARP,
  FX, Voice Tuning, `CLEAR` e round-trip de estado.
- `tests/toolbar_processor_test.cpp`: contagem inicial passou de 72 para 73
  itens visíveis.
- `tools/au-arp-probe.cpp`: opções `--preset` (carrega o XML via ClassInfo) e
  `--program` (Program Change antes do MIDI de teste).

## Null-test executado

Método: AU real do TAL-U-NO-LX 5.1.5 com o preset carregado por
`kAudioUnitProperty_ClassInfo`, e AU KODA com Program Change 72. Mesmo MIDI
(C4–E4–G4, velocity 100), 44,1 kHz, bloco 256, 4 s de nota e 2 s de cauda.

| Medida | TAL | KODA |
| --- | --- | --- |
| RMS | 0.031969 | 0.019986 |
| SHA-256 do render | `4f043580...68bec` | `0f88526b...1e81b` |
| Determinismo (2 renders) | idêntico | idêntico |
| Resíduo null | — | **1.20 dB** |
| Correlação de amostras | — | **0.0578** |

O resíduo e a correlação mostram que o resultado **não é sample-idêntico**. A
sequência de arpejo do KODA foi confirmada no caminho de debug: 60, 64, 67, 72,
76, 79, 84, 88, 91 (C–E–G e oitavas), coerente com o preset. As diferenças
restantes estão no envelope/filtro, no gate/release, na fase e no gate do
arpejador e nas fases de oscilador/chorus.

Arquivos: `evidence/reference-preset/tal-dream.raw`, `koda-slot72.raw`,
`metrics.json`, `tal-spectrum.png`, `koda-spectrum.png`.

## Instalação

- AU: `~/Library/Audio/Plug-Ins/Components/KODA-JUNO-60.component`
- VST3: `~/Library/Audio/Plug-Ins/VST3/KODA-JUNO-60.vst3`
- Pacote com AU/VST3/Standalone: `dist/reference-preset-0.6.1/`
- Backup anterior: `dist/installed-before-reference-preset-20261023-153221/`

`ctest`: 14/14. `auval -v aumu TuCl InAu`: AU VALIDATION SUCCEEDED.

## Limites

- O slot 72 é uma recriação do **Dream Arp**, não do “Arp Magic Synth” original.
- “Sample-idêntico” não foi alcançado; o null-test mede 1.20 dB de resíduo.
- `maxpoly`, MPE, MTS, Voice Mode e outros campos LX sem equivalente KODA não
  foram aplicados.
- O `CLEAR` do slot 72 restaura a tabela de referência; não foi alterado para
  os demais programas.
- O resultado não prova equivalência em DAW, taxas diferentes de 44,1 kHz,
  outros acordes ou todos os modos do arpejador.
