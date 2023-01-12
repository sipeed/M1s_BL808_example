# RUNNING BLAI MODEL CONVERTED FROM TFLITE ON BL808

## TRAIN YOUR MODEL

1) First, follow the rules below to train your model with `TensorFlow`:
   
   | Type        | Operators       | BL808 support                         |
   | ----------- | --------------- | ------------------------------------- |
   | Convolution | Conv            | kernel 1x1,3x3,5x5,7x7<br/>stride 1,2 |
   |             | Pad             | same                                  |
   |             | DepthwiseConv   | yes                                   |
   |             | Deconvolution   | upsample+conv                         |
   | Pooling     | MaxPool         | stride 1,2<br/>size 3                 |
   |             | AvgPool,GAP,GMP | CPU                                   |
   | Activation  | Relu,Relu6      | NPU                                   |
   |             | PRelu,Sigmoid   | CPU                                   |
   | Other       | BN              | CPU                                   |
   |             | Add             | NPU                                   |
   |             | Concat          | NPU, axis=1                           |
   |             | FC              | NPU                                   |
   |             | Mul             | CPU+NPU                               |
   |             | Slice           | CPU                                   |
   
   <mark>ATTENTION: DO NOT TRY TO USE ANY OPERATOR NOT LISTED ABOVE!</mark>
2. Then you will get a model with pretty accuracy, after that, go to [this page on TF](https://www.tensorflow.org/lite/models/convert/convert_models#converting_a_keras_h5_model_) for learning how to convert your TensorFlow model to a TensorFlow Lite model. Pay attention on [this page](https://www.tensorflow.org/lite/performance/post_training_integer_quant) to create a int8 quantized model.
   
   A FEW THINGS ABOUT CONFIGURATION FOR QUANTIZATION TO BE AWARE OF BELOW:
   
   - `converter._experimental_disable_per_channel = True`
   
   - `converter.target_spec.supported_ops = [tf.lite.OpsSet.TFLITE_BUILTINS_INT8]`
   
   - `converter.inference_input_type = tf.int8 ` and `converter.inference_output_type = tf.int8`

3. After all above, you will get a TF model and a TFLITE model. Do a few tests to ensure the TFLITE model still working well just like the TF model.
   
   Like: compare the output from these two models under the same input(of course not the same dtype).

## CONVERT TO BLAI MODEL

1. Get the [blai_toolchain](https://dl.sipeed.com/shareURL/MAIX/M1s/M1s_Dock/8_Toolchain/blai_npu), strcture of the directory shows below(filtered):
   
   ```shell
   ├── BLAI.cfg                            # configuration file
   ├── blai_toolchain                      # executable file for Linux
   ├── BLAI_toolchain.exe                  # executable file for Windows
   ├── cfg                                 # example configuration files
   ├── docs
   │   └── blai_toolchain_User_Manual.pdf  # document for usage
   ├── image                               # images for test
   │   └── a.bmp
   ├── models
   │   └── tflite
   │       └── mnist.tflite           # example tflite model
   ├── names                               # not so important below
   ├── opencv_world420.dll
   ├── pthreadVC2.dll
   ├── tools
   │   ├── cmd.txt
   │   ├── gen_array.py
   │   └── gen_golden.py
   └── yolo_cpp_dll.dll
   ```

2. Modify the BLAI.cfg, contents detail below:
   
   ```textile
   username = mnist                        # output directory for generated model (Modified)
   model = models/tflite/mnist.tflite      # which tflite to be used (Modified)
   data_type = int8                        # int8 quantization
   
   #[app_type] CLASSIFICATION, YOLO, SSD, KWS, CUSTOM, RETINA_FACE, RETINA_PERSON, FR_REGISTRATION, FR_EVALUATION, NONE
   app_type = NONE                         # no post procedure
   
   #[input data_type]: image, file
   input_type = image
   input_data = image/a.bmp                # test image (Modified)
   
   #[input format]: NHWC, NCHW
   input_format = NHWC                     # keep default below
   
   #[memory allocation]
   patch_size = 65536
   patch_num = 45
   ```

3. Just exec ./blai_toolchain or ./blai_toolchain.exe in terminal, then it shows:
   
   ```shell
   PS Z:\blai_toolchain> .\BLAI_toolchain.exe
   names: Using default 'names/imagenet.names'
   mode: Using default 'blai'
   api: Using default 'NPU_SIM'
   version: Using default '0'
   JSON_LOG: Using default '1'
   anchors_num: Using default '6'
         layer          filters   size/strd(dil)        input                output      in1 in2 in3
       0 CONV           12/   1   3 x   3 / 2(1)    28 x  28 x   1  ->   14 x  14 x  12  -1  -9  -9
   
       1 CONV           24/   1   3 x   3 / 2(1)    14 x  14 x  12  ->    7 x   7 x  24   0  -9  -9
   
       2 CONV           48/   1   3 x   3 / 2(1)     7 x   7 x  24  ->    4 x   4 x  48   1  -9  -9
   
       3 CONV           48/   1   3 x   3 / 2(1)     4 x   4 x  48  ->    2 x   2 x  48   2  -9  -9
   
       4 RESHAPE       192/   1   1 x   1 / 1(0)     2 x   2 x  48  ->    1 x   1 x 192   3  -9  -9
   
       5 MATMUL         10/   1   1 x   1 / 1(0)     1 x   1 x 192  ->    1 x   1 x  10   4  -9  -9
   
   INPUT MODE 0
   Network Inference Progess: 100.00 / 100.00
   ```
   
   0 error means conversion success, but you should check again the layers above(yours) whose params are all compatible.

## BEFORE DEPLOY TO BL808 NPU

1. You should check the output of the 'blai_toolchain' simulator against the results of the tflite runtime. Use the same input image: `a.bmp`(yours).

2. SIM outputs  in  `<username>/BLAI_output/output_layer_data.h`, details below:
   
   ```c
   static const uint8_t golden[] = {
       0x6d, 0x87, 0x7f, 0x7c, 0x6e, 0x57, 0x4e, 0xab, 0x6a, 0x7c, 0x00, 0x00
   };
   ```
   
   And the `0x93` is the biggest value whose index is `7` count from `0`. Also compare the relative difference for each one.
   
   For example, my tflite output is:
   
   ```python
   # out
   [-19   7  -1  -4 -18 -41 -50  43 -22  -4]
   # list(map(hex,(out).astype('uint8')))
   ['0xed', '0x7', '0xff', '0xfc', '0xee', '0xd7', '0xce', '0x2b', '0xea', '0xfc']
   # list(map(hex,(out0+128).astype('uint8')))
   ['0x6d', '0x87', '0x7f', '0x7c', '0x6e', '0x57', '0x4e', '0xab', '0x6a', '0x7c']
   ```
   
   You will found they are different,  but one thing that's remarkable is that they're  just off by a certain number `128` or `0x80`. And that's OK, because SIM in `blai_toolchain` will output the value with offset of 128 default.
   
   <mark>TIPS: OUTPUT CHANNEL WILL BE AUTO ALIGNED TO MULTIPLES OF 4 IF NOT 1</mark>
   
   Just leave the auto filled value alone.

## DEPLOY TO BL808 NPU

1. After the last step, you have got a model in `<username>/BLAI_input/model.h`. Use the python script below to convert it to a `blai` file that can be put into the file system in M1s Dock.  Remember rename `model.blai` into anything you want.
   
   ```python
   import numpy as np
   open("model.blai", "wb").write(np.array(list(map(eval, open("model.h").read().split("static const uint8_t blai_model_bin[] = {")[1].split("};")[0].replace(" ", "").replace("\n", "").split(","))), 'uint8'))
   ```

2. Power up the  M1s Dock without any more operation, and ensure the port labeled OTG was plugged to your PC. A removable disk will be found, and it should be less than 4MB as a part of the onboard FLASH. Put the `blai` model in anywhere you want, and here I put it into `models/mnist.blai`.

3. Now, write a few lines of code to make the model play a role like classify or detect something.
   
   <mark>TIPS: FOLLOW THE DEMO WE RELEASED AND TAKE JUST A LITTE CHANGE.</mark>

## FINAL

    IF YOU STILL HAVE ANY PROBLEM OR CONFUSION, PLEASE LET ME KNOW WITH AN ISSUE, I WILL TAKE A VIEW AND TRY TO HELP YOU. AND ALSO HOPE YOU TO SHARE MORE FUNNY MODEL TO MAKE MORE PEOPLE ENJOY IT AS YOU.
