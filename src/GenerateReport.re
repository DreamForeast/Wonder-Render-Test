open RenderTestDataType;

open JimpType;

open Node;

open Js.Promise;

let _buildDiffImagePath = (targetAbsoluteFileDir, caseText) =>
  Path.join([|targetAbsoluteFileDir, caseText ++ ".png"|]);

let _generateDiffImages = (targetAbsoluteFileDir: string, compareResultList) =>
  compareResultList
  |> List.fold_left(
       (promise, (caseText, currentImagePath, correctImagePath, diffImage, currentTestDataItem)) =>
         promise
         |> then_(
              (list) => {
                let diffImagePath = _buildDiffImagePath(targetAbsoluteFileDir, caseText);
                make(
                  (~resolve, ~reject) =>
                    Jimp.writeCb(
                      diffImagePath,
                      () =>
                        [@bs]
                        resolve([
                          (
                            caseText,
                            currentImagePath,
                            correctImagePath,
                            diffImagePath,
                            currentTestDataItem
                          ),
                          ...list
                        ]),
                      diffImage
                    )
                )
              }
            ),
       [] |> resolve
     );

let _buildHeadStr = () => GenerateHtmlFile.buildHeadStr("render test");

let _buildFootStr = () => {|</body>
        </html>|};

let _buildFailCaseListHtmlStr = (targetAbsoluteFilePath, imageFilePathDataList) =>
  imageFilePathDataList
  |> List.map(
       ((caseText, currentImagePath, correctImagePath, diffImagePath, _)) => (
         caseText,
         GenerateHtmlFile.getRelativeFilePath(targetAbsoluteFilePath, currentImagePath),
         GenerateHtmlFile.getRelativeFilePath(targetAbsoluteFilePath, correctImagePath),
         GenerateHtmlFile.getRelativeFilePath(targetAbsoluteFilePath, diffImagePath)
       )
     )
  |> List.fold_left(
       (resultStr, (caseText, currentImagePath, correctImagePath, diffImagePath)) => {
         let debugFilePath = "./" ++ GenerateDebug.buildDebugHtmlFileName(caseText);
         resultStr
         ++ {j|<section>
                    <h3>$caseText</h3>
                        <a href="$debugFilePath" target="_blank"><img class="correct-img" src="$correctImagePath"/></a>
                        <a href="$debugFilePath" target="_blank"><img class="current-img" src="$currentImagePath"/></a>
                        <a href="$debugFilePath" target="_blank"><img class="diff-img" src="$diffImagePath"/></a>
                </section>
                    |j}
       },
       ""
     );

let _generateCssFile = (filePath) =>
  {|
img.correct-img, img.current-img, img.diff-img{
    width:33%;
    height:33%;
};|}
  |> WonderCommonlib.NodeExtend.writeFile(filePath);

let removeFiles = (reportFilePath) =>
  Fs.existsSync(reportFilePath) ?
    WonderCommonlib.NodeExtend.rmdirFilesSync(reportFilePath |> Path.dirname) : ();

let generateHtmlFile = (targetAbsoluteFilePath: string, (renderTestData, compareResultList)) =>
  compareResultList
  |> _generateDiffImages(Path.dirname(targetAbsoluteFilePath))
  |> then_(
       (imageFilePathDataList) => {
         let htmlStr =
           _buildHeadStr()
           ++ "\n<body>\n"
           ++ GenerateHtmlFile.buildImportScriptStr(targetAbsoluteFilePath, renderTestData)
           ++ _buildFailCaseListHtmlStr(targetAbsoluteFilePath, imageFilePathDataList)
           ++ GenerateHtmlFile.buildFootStr();
         htmlStr |> WonderCommonlib.NodeExtend.writeFile(targetAbsoluteFilePath);
         _generateCssFile(
           GenerateHtmlFile.buildDebugCssFilePath(targetAbsoluteFilePath |> Path.dirname)
         );
         htmlStr |> resolve
       }
     );