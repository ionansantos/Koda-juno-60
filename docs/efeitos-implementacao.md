# Unison, Delay, Reverb e FX LOCK — implementação 0.4.0

Implementado no KODA-JUNO-60 para Apple Silicon, AU, VST3 e standalone. Os 58 IDs anteriores permanecem; os 14 novos parâmetros totalizam 72. O painel contém 12 knobs e dois botões com automação bidirecional. A cadeia é Unison dentro das vozes → filtros/envelopes → chorus → delay → reverb. Não foram copiados executáveis do TAL.

## Controles e presets

Unison: 1 voz desligado, depois 3–7 osciladores por voz; DET quadrático com pares ±1/±3/±5; WIDTH altera os pesos estéreo. Delay: WET, TIME, SYNC com 21 divisões, SPREAD, TONE e FDB. Reverb: WET, SIZE, DELAY quadrático e TONE. As curvas e a topologia se baseiam na [análise REA](lx-unison-delay-reverb-rea.md).

FX LOCK é global e preserva os dez parâmetros de Delay/Reverb ao trocar programas. Unison acompanha o programa. A restauração completa de sessão restaura todos os parâmetros, inclusive LOCK. Sessões antigas recebem os novos efeitos desligados. PANIC e reinicialização limpam as caudas.

## Validação

Nove testes CTest passaram, cobrindo regressão do arpejador/MIDI/presets, Unison em 44,1/48/96 kHz, delay e reverb em múltiplas taxas, estabilidade, reset, estados antigos/completos, presets/LOCK e ligação dos 14 controles do editor. A captura do editor está em `evidence/lx-effects/runtime/panel.png`. `auval -v aumu TuCl InAu` terminou com AU VALIDATION SUCCEEDED. Esta rodada não controlou uma DAW gráfica nem validou VST3 dentro de uma DAW.

Foram renderizados os AUs reais TAL-U-NO-LX 5.1.5 e KODA em 31 cenários, em 44,1 e 48 kHz, com os parâmetros registrados, 1 segundo de preroll e controles secos por instrumento. Nenhum arquivo teve amostras não finitas, silêncio inesperado ou contagem incorreta. As sete verificações por instrumento passaram nas duas taxas: ordem dos tempos livres, sincronismo em semínima, abertura estéreo, predelay, direção espectral dos dois TONE e maior cauda com SIZE alto. Esses critérios verificam comportamento; não estabelecem equivalência sonora.

Em 44,1 kHz, TIME=.3 começou em 342,608 ms no TAL e 342,857 ms no KODA; SYNC em 499,025 e 499,274 ms. Em 48 kHz as diferenças foram aproximadamente 0,21 ms. A queda normalizada das repetições concorda de perto. WIDTH máximo teve razão side/mid de 1,0018 e 1,0010 em 44,1 kHz. Contagem de vozes e frequências de detune também concordaram nas medições anteriores.

O Reverb mantém diferenças de difusão, modulação, distribuição estéreo e cauda. Na versão final, em 44,1 kHz o RMS da cauda padrão foi 0,005289 no TAL e 0,006578 no KODA; SIZE máximo foi 0,007257 e 0,010299. Parte decorre do ganho seco dos instrumentos (KODA aproximadamente 1,155×); mesmo após normalização resta diferença. Não foi aplicado ganho arbitrário para esconder isso. A inicialização da rampa foi corrigida conforme o código recuperado, mas as sementes aleatórias originais e diferenças aritméticas dos filtros permanecem desconhecidas. Fase dos osciladores influencia especialmente notas curtas em Unison e a cadeia serial. Portanto, esta é uma reconstrução funcional baseada em evidências, sem alegação de áudio idêntico ao TAL.

Evidência final: `evidence/lx-effects/audio-44100-final/results.json` e `audio-48000-final/results.json`, com hashes dos binários, parâmetros e arquivos de áudio brutos. As matrizes levaram 10,78 e 11,27 segundos; CPU, pico de memória e custo de tokens não foram medidos. Relatórios anteriores são históricos.

## Instalação

AU e VST3 foram assinados ad hoc e verificados, instalados nas pastas de usuário. Pacote com standalone: `dist/fx-0.4.0/`. Backup anterior: `/Users/koda/Documents/tal-un-no-60-clone/dist/installed-before-fx-20261019-011555`. Identidades instaladas: `evidence/lx-effects/runtime/installation.json`. Reinicie a DAW para carregar 0.4.0.
