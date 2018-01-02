open Wonder_jest;

let _ =
  describe(
    "test compare",
    () =>
      Expect.(
        Expect.Operators.(
          Sinon.(
            Js.Promise.(
              Node.(
                RenderTestDataType.
                  /* afterEach(() => NodeExtend.rmdirFilesSync(Path.join([|Process.cwd(), "./test/image"|]))); */
                  (
                    testPromiseWithTimeout(
                      "test compare correct and wrong image",
                      () =>
                        RenderTestData.(
                          Comparer.compare(renderTestData)
                          |> then_(
                               (list) => {

                                 WonderCommonlib.DebugUtils.log(list |> List.length) |> ignore;

                                 /* WonderCommonlib.DebugUtils.log(List.nth(list,1)) |> ignore; */

                                 1 |> expect == 1 |> resolve
                               }
                             )
                        ), 160000
                    )
                  )
              )
            )
          )
        )
      )
  );