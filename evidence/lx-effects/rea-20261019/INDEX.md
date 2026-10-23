# REA — Unison, Delay, Reverb e FX LOCK

TAL-U-NO-LX 5.1.5 ARM64. Identidade: [identity.json](identity.json). Respostas completas e pseudocódigo preservados; pseudocódigo não é fonte original nem prova de execução.

| Registro | Operação | Evidence ID |
| --- | --- | --- |
| [010-effects-symbols.json](010-effects-symbols.json) | `search_procedures` | `ev_bffad944682ed4d9c38c029ba883cab373257b9d7f876cf6b76ab5f5c2d10b32` |
| [020-delay-parameters.json](020-delay-parameters.json) | `procedure_pseudo_code` | `ev_fbf80dc766d1a5f6434e3d5317189f8d532fe9a134aeb0cfba2227141966be03` |
| [021-reverb-parameters.json](021-reverb-parameters.json) | `procedure_pseudo_code` | `ev_9a22a6f7e67edfa212641f4ffa78ef5810b2f46174ebbef2eccf9224b6afdfa4` |
| [022-delay-process.json](022-delay-process.json) | `procedure_pseudo_code` | `ev_c66cc74224332266e6d0cf065b508b0a86e548bdbafad857aebe5ae8d1a47415` |
| [023-reverb-init.json](023-reverb-init.json) | `procedure_pseudo_code` | `ev_f3b1a64fd9582b247fd021fccfb352bb07c940649b0f062d5b43943251bdf23a` |
| [024-delay-constructor.json](024-delay-constructor.json) | `procedure_pseudo_code` | `ev_f52a0412c1582ea6a798e3309194deebdc2dd87749478284ce7fcef501953347` |
| [025-delay-sync.json](025-delay-sync.json) | `procedure_pseudo_code` | `ev_f5e1596214b44679dce6d54d73e64ae208619414cca43fa5e9cc13053fb5b926` |
| [026-state-load.json](026-state-load.json) | `procedure_pseudo_code` | `ev_4ccc6f5546b68fde0633921899eefd0e6001a3e83ba725a8faac97f9aa0141d6` |
| [027-voice-note-on.json](027-voice-note-on.json) | `procedure_pseudo_code` | `ev_1723a18ae80153345f9c679474aa4cd97979dee71806db63c5bb2ccdf2ce59a9` |
| [028-parameter-layout.json](028-parameter-layout.json) | `procedure_pseudo_code` | `ev_3efa1bf1745d192d41fd8f7ded81d882de58b9e24d34154284cc5b81371ba2ab` |
| [029-voice-start.json](029-voice-start.json) | `procedure_pseudo_code` | `ev_f30f0544f2c25833ae0c40eafc23872f6234c1c9fd868d659ae84f08f70cdc58` |
| [030-voice-process.json](030-voice-process.json) | `procedure_pseudo_code` | `ev_5c737013be92f29c616768f0fd9ad55d7869f9639299827e2ff505a22700ab02` |
| [031-delay-assembly.json](031-delay-assembly.json) | `procedure_assembly` | `ev_2455464ee91f60d902eb102aaff8b0ffdc64deafd0d840fea21b4e396a1610a5` |
| [032-preset-helper.json](032-preset-helper.json) | `procedure_pseudo_code` | `ev_213759c5c11ada636cd08df271c695b3fa07ea579af8b1fc55f593a891f4f0aa` |
| [033-vco-process.json](033-vco-process.json) | `procedure_pseudo_code` | `ev_c6668d377921b8ed200d5570ea6f626670f4b94bcfa1d3da78dd16fae662456a` |
| [034-delay-samplerate.json](034-delay-samplerate.json) | `procedure_pseudo_code` | `ev_5c80fd00c9048395e1d7194a1d219c89b8c4e52662d045f004d2e78bcbafe0c8` |
| [035-engine-construction.json](035-engine-construction.json) | `procedure_pseudo_code` | `ev_47214377d6aa111345191a6efc3a801a30a06ee9e7ab4c133ad914f6d0ba28d9` |
| [036-unison-formatter-vtable.json](036-unison-formatter-vtable.json) | `read_bytes` | `ev_19476ed2a37461fafab856df5d7a87ef46933a59451ff0a6fd6294926bbf5071` |
| [037-reverb-filter-vtables.json](037-reverb-filter-vtables.json) | `read_bytes` | `ev_d3628c27ffbbf9ccd2341e71ce11a898152a61a46f1c7364b5b4976d8a8bd469` |
| [038-unison-voices-display.json](038-unison-voices-display.json) | `procedure_pseudo_code` | `ev_35f04b15c2f4c4c4d0deac03a8ff728e84b888864c4253fca217830e6ba3d6ad` |
| [039-reverb-highpass-coefficients.json](039-reverb-highpass-coefficients.json) | `procedure_pseudo_code` | `ev_6e637ecbe6e232f8384e3e48239c1a3a583ed5049f2711582b53d6d1adc548d9` |
| [040-reverb-lowpass-coefficients.json](040-reverb-lowpass-coefficients.json) | `procedure_pseudo_code` | `ev_e9b842d444fa82dde14e717e7f6e5460d91cb2455a940d4b1edc63ccc0a89b10` |

Registros anteriores reutilizados, mesmo SHA-256 e perfil Ghidra:

- [processBlock](../../arpeggiator/rea-20261018-playback/10-process.json): `ev_71e37182bed5a7abc82d87ad3081854b53faf5cccbcbefb7c91fb58f45379fa0`.
- [SynthEngine::parameterChanged](../../arpeggiator/rea-20261018-unrestricted/07-parameters.json): `ev_088cae064f597d74469a9ff0010beef35d7d980fc1cd55a3a37cf985df88dc29`.

[Bundle completo da nova sessão](evidence-bundle.json). Os dois registros históricos acima permanecem nos seus arquivos de origem.
