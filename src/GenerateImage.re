open GenerateImageType;

open RenderTestDataType;

open Node;

open Performance;

open Js.Promise;

open Puppeteer;

let _evaluateScript: (string, array(int)) => unit = [%bs.raw
  {|
   function(bodyFuncStr, timePathArr) {
        wd.startDirector = function(state){
            return state
        };
     window.performance.now = function(){
       return 0
     }

var bodyFunc = new Function(bodyFuncStr);

var state = bodyFunc();

 var state = wd.initDirector(state);

var state = timePathArr.reduce(function(state, time){
  return wd.loopBody(time, state)
}, state);
   }
    |}
];

let _addScript = ({scriptFilePathList: commonScriptFilePathList}, scriptFilePathList, promise) =>
  (
    switch scriptFilePathList {
    | None => commonScriptFilePathList
    | Some(scriptFilePathList) => scriptFilePathList @ commonScriptFilePathList
    }
  )
  /* |> List.map((filePath) => Path.join([|Process.cwd(), filePath|])) */
  |> List.fold_left(
       (promise, scriptFilePath) =>
         promise
         |> then_(
              (page) =>
                /* WonderCommonlib.DebugUtils.log(scriptFilePath) |> ignore; */
                page
                |> Page.addScriptTag({
                     "url": Js.Nullable.empty,
                     "content": Js.Nullable.empty,
                     "path": Js.Nullable.return(scriptFilePath)
                   })
                |> then_((_) => page |> resolve)
            ),
       promise
     );

let _buildImageName = (imageName, timePath) =>
  imageName ++ "_" ++ (timePath |> Array.of_list |> Js.Array.joinWith("_")) ++ ".png";

let buildImagePath = (imageType, imageName, imagePath, timePath) =>
  switch imageType {
  | CORRECT(dir) =>
    Path.join([|Process.cwd(), imagePath, dir, _buildImageName(imageName, timePath)|])
  | CURRENT(dir) =>
    Path.join([|Process.cwd(), imagePath, dir, _buildImageName(imageName, timePath)|])
  };

let _createImageDir = (generateFilePath: string) => {
  let dirname = generateFilePath |> Path.dirname;
  dirname |> Fs.existsSync ?
    () : NodeExtend.mkAlldirsSync(generateFilePath |> Path.dirname) |> ignore;
  ()
};

let getAllImagePathDataList = ({commonData, testData}, imageType) =>
  testData
  |> List.fold_left(
       (list, {name, distance, diffPercent, threshold, frameData} as testDataItem) =>
         (
           frameData
           |> List.fold_left(
                (list, {timePath}) => [
                  (buildImagePath(imageType, name, commonData.imagePath, timePath), testDataItem),
                  ...list
                ],
                []
              )
         )
         @ list,
       []
     );

let generate = (browser, {commonData, testData}, imageType) =>
  testData
  |> List.fold_left(
       (promise, {bodyFuncStr, name, frameData, scriptFilePathList}) =>
         frameData
         |> List.fold_left(
              (promise, {timePath}) =>
                promise
                |> then_(
                     (browser) =>
                       browser
                       |> Browser.newPage
                       |> _addScript(commonData, scriptFilePathList)
                       |> then_(
                            (page) =>
                              page
                              |> Page.evaluateWithTwoArgs(
                                   [@bs] _evaluateScript,
                                   bodyFuncStr,
                                   timePath |> Array.of_list
                                 )
                              |> then_(
                                   (_) => {
                                     let path =
                                       buildImagePath(
                                         imageType,
                                         name,
                                         commonData.imagePath,
                                         timePath
                                       );
                                     _createImageDir(path);
                                     page
                                     |> Page.screenshot(
                                          ~options={
                                            /* "clip":
                                               Js.Nullable.return({
                                                 "x": 0.,
                                                 "y": 0.,
                                                 "width": 300.,
                                                 "height": 150.
                                               }), */
                                            "clip": Js.Nullable.empty,
                                            "fullPage": Js.Nullable.return(false),
                                            "omitBackground": Js.Nullable.return(false),
                                            "path": Js.Nullable.return(path),
                                            "quality": Js.Nullable.empty,
                                            "_type": Js.Nullable.return("png")
                                          },
                                          ()
                                        )
                                   }
                                 )
                              |> then_(
                                   (_) => page |> Page.close |> then_((_) => browser |> resolve)
                                 )
                          )
                   ),
              promise
            ),
       browser |> resolve
     );