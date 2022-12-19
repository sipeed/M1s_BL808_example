# 0.Introduce

A model for Tom&Jerry classfication based on mobilenetv2 is built on MaixHub.

# 1.Get Started

```shell
cd c906_app
./build.sh tom_and_jerry_classification_demo
# then generated `build_out/d0fw.bin`
```

Darg-n-drop it to M1sDock, and then reset the board.

A nearly 8MB removable Udisk besed on onboard flash will appear, and then visit [model on MaixHub](https://maixhub.com/model/zoo/127) and press download to fetch the model named `*.blai` or `*.sblai` (encrypted). After that, mkdir `models` on this Udisk, and place model file into it with new name `tj.blai`.

```shell
└── models
    └── tj.blai
```

Final reset, it will work well.

# 2.Furthermore

more details are available here, take a look at `main.c` and you can learn how to deploy another model from MaixHub.

# 4.Need Help

please post an issue on our github repository if meet any problems.