# Models

[Source code](https://github.com/yuyuranium/ARC-AIoT-driver-feedback-system/blob/main/train/Colab%20Notebooks/SafeDriver.ipynb)

## Platform

We use tensorflow functional API to build our models. Tensorflow functional API provides us a more flexibility  way to build models while using self-defined residual network and branch structure.

The tensorflow keras models are converted to tensorflow lite integer 8 models before deploying on Himax WE-1 Plus EVB.

## Preparation

### Features

* 3-axis acceleration
* 3-axis jerk

The acceleration is measured by the 3-axis accelerometer on  Himax WE-1 Plus EVB with 25 Hertz sampling rate, and jerk is the derivative of acceleration.

<img src='../img/plot_data.png'>

### Motion (Classification)

We have six classes of labels, each label is mapped to a specific driving  behavior.

| Label | Motion     | Condition of vehicle    |
| :---: | ---------- | ----------------------- |
|   0   | Idle       | Stay still              |
|   1   | Cruise     | Cruise with fixed speed |
|   2   | Accelerate | Speed up                |
|   3   | Brake      | Speed down              |
|   4   | Left       | Turn left               |
|   5   | Right      | Turn right              |

## Work flow of data preprocessing

<h3 id="workflow1">Separate training and valid data</h4>

We separate the training and valid files in the beginning, which means the data from a single file are either in training dataset or valid dataset. In this step, the features in CSV files are read and the labels are converted to one-hot format.

<h3 id="workflow2">Split data by classes</h4>

Separating the data by their labels, observing the size of each class of data and deciding how to balance the data by augmentation.

<h3 id="workflow3">Data augmentation</h4>

Using rotation matrix and multiply with some original data to generate new training data.

<h3 id="workflow4">Normalization</h4>

Calculating the mean values and standard deviations of each features, and normalize the data using the format: 

<img src="http://chart.googleapis.com/chart?cht=tx&chl= N(x) = (x - \bar{x})/\sigma" style="border:none;">

<h3 id="workflow5">Split sequences for training models</h4>

We choose a value to be the length of our training sequences, and generate the sequences for training and validation. The output *x* is the training sequence, containing the consequent time series signals of length *30*, *y1* is the category of motion in the duration of  *x*, and *y2* is the information after the time of  *x*. Both models has the common input training data *x* , but *y1* is for **classifier** model, and *y2* is for **predictor** model.

| Output | Meaning                                                      |
| ------ | ------------------------------------------------------------ |
| x      | Time series of information from t<sub>start</sub> to t<sub>start+n</sub>. |
| y1     | The class of motion of t<sub>start</sub> to t<sub>start+n.</sub> |
| y2     | The information of t<sub>start+n+1</sub>.                    |

<img src='../img/split_data.png' width=80% height=80%>

## Classifier model

### Usage

The classifier is a model which classifies the input sequences into six different categories of motions. The input of classifier model is the acceleration and jerk of the last *n* samples, and the output is one of the above six state.

| Input                                        | Output                      |
| -------------------------------------------- | --------------------------- |
| Latest *n* samples of acceleration and jerk. | Category of current motion. |

### Structure

#### Inception cell

An inception cell contains two primary branches, one is bottleneck branch. A bottleneck layer is used to reduce the dimensionality of the inputs,  following  by three Conv1D layers of different kernel size to specify long-term, mid-term and short-term features. The other one is Maxpooling1D branch and follow by a Conv1D layer with kernel size equals to 1. The four Conv1D layer are concatenate along the depth dimension.



<img src="../img/inception_cell.png" width=70% height=70%>



#### Complete model

A complete classifier model is built by three inception cell, including one residual net work from input layer to the second

inception cell, and the flatten and output layers.

<img src="../img/Classifier_diagram.png" width=50% height=50%>

## Predictor model

### Usage

The predictor model predict the future values of 3-axis acceleration by observing the last *n* samples.

| Input                                       | Output                                                |
| ------------------------------------------- | ----------------------------------------------------- |
| Latest *n* samples of acceleration and jerk | The predict values of 3-axis acceleration next sample |

### Structure

The predictor model is built by several Conv1D and Maxpooling1D layers in the first half, and fully connected layers in the second half.

<img src="../img/Predictor_diagram.png" width=30% height=20%>

### Demonstration

<img src='../img/Predictor_waveform.png'>



## Deploy Steps

### Convert to tensorflow lite models

The tensorflow models must convert to tensorflow lite models before deployed to Himax WE-1 Plus EVB. Here we choose int8 as the input and output type for out models for the purpose of lighter weight and faster calculation. TFLIteConverter can be used to convert tensorflow models to tensorflow lite  models directly, and after the models are converted, we can use tf.lite.Interpreter to call our models. We can reshape the input and output while via interpreter and get the zero points and scale of int8 models.
