# Audio Unit compilado e instalado

Data:14/10/2026. ProjetoJUCE8.0.15, CMake4.4.4, AppleClang21,
macOS27.0.1, arquiteturaarm64, deployment target11.0.

- Build: `build/TalUNo60Clone_artefacts/Release/AU/U-No-60 Study.component`
- Instalado: `/Users/koda/Library/Audio/Plug-Ins/Components/U-No-60 Study.component`
- Nome: `Independent Audio: U-No-60 Study`
- AudioComponent: tipo`aumu`, subtype`TuCl`, manufacturer`InAu`
- BundleID: `audio.independent.taluno60clone`
- Versão:0.1.0
- Assinatura local ad-hoc: criada porcodesign e verificada com`--deep --strict`.
- AU_SANDBOX_SAFE=TRUE; nenhuma declaração de acesso a rede/arquivos foi necessária.

JUCE e CMake foram baixados das versões oficiais pelo navegador, porque o download
do terminal falhou porDNS. Dependências estão isoladas em`external/JUCE` e`.tools`;
não foi feita instalação global das ferramentas.

Fontes: [JUCE8.0.15](https://github.com/juce-framework/JUCE/releases/tag/8.0.15),
[CMake4.4.4](https://github.com/Kitware/CMake/releases/tag/v4.4.4).
O ZIPJUCE confere com SHA256 publicado:
`4ed8602cc3e7bfa7cac7d61280ff2a0f6e1e0faf17824adf1243ad4e7a6de19e`.

## Verificação

O buildAU terminou com sucesso: [log final](../evidence/au-build-final.log).
CTest: **2/2 passaram**, com assertions preservadas mesmo noRelease:
[log](../evidence/au-ctest.log).

O`auval` executado pelo terminal desta sessão não enumerou nenhumAudioUnit;
não foi possível concluir sua validação por esse caminho. Seu erro de descoberta
está preservado em[auval.log](../evidence/auval.log), e a lista vazia em
[au-list.log](../evidence/au-list.log). Não se afirma queauvalpassou.

**Confirmação em host:** LogicPro12.3.1 realizou atualização de informaçõesAudioUnit
ao abrir. A observação deUI retornou um editor`U-No-60 Study` carregado na janela
`Deluxe Classic`, com seletor`1 Startup`, controlesVOLUME0.4140793,
MASTERPITCH0.5,CUTOFF1,SaveCPUligado e os parâmetros internos. Isso confirma
reconhecimento e instanciação doAudioUnit/editor peloLogic. Não estabelece
equivalência de áudio aoVSToriginal.

Recompilar: `sh tools/build-au.sh`.
Recompilar e instalar: `sh tools/build-au.sh --install`.
Após atualização, reinicieLogic para carregar o novo binário.
