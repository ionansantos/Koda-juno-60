# Comparação necessária com o original

Este ambiente não executa o plugin PowerPC/i386 em um host compatível. Antes de
chamar o projeto de equivalente, gere referências do arquivo fornecido em um
host Intel32bit que consiga carregá-lo. Use o mesmo sample rate, BPM, programa,
valores de parâmetros, sequência MIDI e posição de cada evento em samples.

1. Renderize presets0..119 a44100/48000Hz. Registre o estado de rand/seed quando
   possível; ruído, randomLFO e drift tornam comparação direta não determinística.
2. Compare notaMIDI69 velocity100 noteOff após44100samples, com todos os efeitos
   desligados; compare comprimento, pitch, ganho, espectro e transientes.
3. Ative saw/pulse/sub individualmente; variePWM e modosLFO/envelope/manual.
4. Varra cutoff/resonance/keytrack e as polaridades; compare modoCPU0/1.
5. Meça os estágiosADSR nos extremos0/1 e em valores intermediários. Inclua
   retrigger durante release, roubo de voz e sete notas simultâneas.
6. Compare cada índiceLFO0..4 e sync0..12, mudançasBPM antes/depois de noteOn,
   delay/fade, chorusI/II/ambos e tails após a última nota.
7. Teste pitchbend pelos dois bytes separadamente; o original sobrescreve o cálculo
   doLSB e usa(MSB-64)/64. Teste ProgramChange, CCmapeados, CC120..127 e MIDIlearn.
8. Faça round-trip de bancos/presets/estado e automação em blocos de tamanhos
   diferentes. Valide que o índice de programa e os47valores não mudam por filtro
   ou nomes duplicados.

Preserve WAVs sem normalização, hashes do binário e do banco, dump dos parâmetros,
MIDI, versão dohost, buffer size e ambiente. Registre erroRMS, pico do erro,
diferença de espectro e eventuais offsets de samples; trate caminhos com rand
como comparação estatística até recuperar estado/seed. Separe diferença de
topologia, diferença numérica e diferença dohost.

// TODO verificar: concluir estes testes com renderizações reais do original.
