open RenderTestDataType;

open JimpType;

open Node;

open Js.Promise;

let _buildDiffImagePath = (targetAbsoluteFileDir, caseText) =>
  Path.join([|targetAbsoluteFileDir, caseText, ".png"|]);

let _generateDiffImages = (targetAbsoluteFileDir: string, compareResultList) =>
  compareResultList
  |> List.fold_left(
       (promise, (caseText, currentImagePath, correctImagePath, diffImage)) =>
         promise
         |> then_(
              (list) => {
                let diffImagePath = _buildDiffImagePath(targetAbsoluteFileDir, caseText);
                /* todo wait for write finish? */
                Jimp.write(diffImagePath, diffImage);
                [(caseText, currentImagePath, correctImagePath, diffImagePath), ...list] |> resolve
              }
            ),
       [] |> resolve
     );

let _getRelativeFilePath = (fromAbsoluteFilePath, toAbsoluteFilePath) =>
  Path.relative(~from=fromAbsoluteFilePath, ~to_=toAbsoluteFilePath);

let _buildHeadStr = () => {|<!DOCTYPE html>
   <html lang="en">
   <head>
     <meta charset="UTF-8">
     <title>Wonder.js render test</title>
     <link rel="stylesheet" href="./report.css"/>
   </head>|};

let _getAllScriptFilePathList = ({commonData, testData}) : list(string) =>
  testData
  |> List.fold_left(
       (resultList, {scriptFilePathList}: testDataItem) =>
         switch scriptFilePathList {
         | None => resultList
         | Some(list) => list @ resultList
         },
       commonData.scriptFilePathList
     );

let _buildScriptStr = (targetAbsoluteFilePath, renderTestData) =>
  renderTestData
  |> _getAllScriptFilePathList
  |> List.map((scriptFilePath) => _getRelativeFilePath(targetAbsoluteFilePath, scriptFilePath))
  |> List.fold_left(
       (resultStr, scriptFilePath) => resultStr ++ {j|<script src=$scriptFilePath/>
|j},
       ""
     );

let _buildFootStr = () => {|</body>
        </html>|};

let _buildFailCaseListHtmlStr = (targetAbsoluteFilePath, imageFilePathDataList) =>
  imageFilePathDataList
  |> List.map(
       ((caseText, currentImagePath, correctImagePath, diffImagePath)) => (
         caseText,
         _getRelativeFilePath(targetAbsoluteFilePath, currentImagePath),
         _getRelativeFilePath(targetAbsoluteFilePath, correctImagePath),
         _getRelativeFilePath(targetAbsoluteFilePath, diffImagePath)
       )
     )
  |> List.fold_left(
       (resultStr, (caseText, currentImagePath, correctImagePath, diffImagePath)) =>
         /* todo add debug a href */
         resultStr
         ++ {j|<section>
                    <h3>$caseText</h3>
                        <image class="correct-image" src=$correctImagePath/>
                        <image class="current-image" src=$currentImagePath/>
                        <image class="diff-image" src=$diffImagePath/>
                </section>
                    |j},
       ""
     );

let generateHtmlFile = (targetAbsoluteFilePath: string, (renderTestData, compareResultList)) =>
  compareResultList
  |> _generateDiffImages(Path.dirname(targetAbsoluteFilePath))
  |> then_(
       (imageFilePathDataList) => {
         let htmlStr =
           _buildHeadStr()
           ++ "<body>\n"
           ++ _buildScriptStr(targetAbsoluteFilePath, renderTestData)
           ++ _buildFailCaseListHtmlStr(targetAbsoluteFilePath, imageFilePathDataList)
           ++ _buildFootStr();
         htmlStr |> Fs.writeFileAsUtf8Sync(targetAbsoluteFilePath);
         htmlStr |> resolve
       }
     );