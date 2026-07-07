#include "../include/AIUtil/ImageRecognizer.h"
#include "../include/AIUtil/ImageValidation.h"

#include <algorithm>
#include <cmath>
#include <opencv2/dnn.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

ImageRecognizer::ImageRecognizer(
    const std::string& modelPath,
    const std::string& labelPath)
    : env(ORT_LOGGING_LEVEL_WARNING, "ImageRecognizer")
{
    Ort::SessionOptions sessionOptions;
    sessionOptions.SetIntraOpNumThreads(1);
    sessionOptions.SetGraphOptimizationLevel(
        GraphOptimizationLevel::ORT_ENABLE_EXTENDED);

    session = std::make_unique<Ort::Session>(
        env,
        modelPath.c_str(),
        sessionOptions);
    allocator = std::make_unique<Ort::AllocatorWithDefaultOptions>();

    input_name = session->GetInputNameAllocated(0, *allocator).get();
    output_name = session->GetOutputNameAllocated(0, *allocator).get();
    input_shape =
        session->GetInputTypeInfo(0).GetTensorTypeAndShapeInfo().GetShape();
    if (input_shape.size() != 4 || input_shape[2] <= 0 || input_shape[3] <= 0)
    {
        throw std::runtime_error("Unsupported image model input shape");
    }
    input_height = static_cast<int>(input_shape[2]);
    input_width = static_cast<int>(input_shape[3]);
    LoadLabels(labelPath);
}

void ImageRecognizer::LoadLabels(const std::string& labelPath)
{
    std::ifstream input(labelPath);
    if (!input.is_open())
    {
        throw std::runtime_error("Failed to open label file: " + labelPath);
    }

    std::string line;
    while (std::getline(input, line))
    {
        if (!line.empty())
        {
            labels.push_back(line);
        }
    }
    if (labels.empty())
    {
        throw std::runtime_error("No labels loaded from file: " + labelPath);
    }
}

ImageRecognizer::PredictionResult ImageRecognizer::PredictFromFile(
    const std::string& imagePath)
{
    const cv::Mat image = cv::imread(imagePath);
    if (image.empty())
    {
        throw std::runtime_error("Failed to load image: " + imagePath);
    }
    return PredictFromMat(image);
}

ImageRecognizer::PredictionResult ImageRecognizer::PredictFromBuffer(
    const std::vector<unsigned char>& imageData)
{
    image_validation::validateEncodedImage(imageData);
    const cv::Mat image = cv::imdecode(imageData, cv::IMREAD_COLOR);
    if (image.empty())
    {
        throw std::runtime_error("Failed to decode image from buffer");
    }
    return PredictFromMat(image);
}

ImageRecognizer::PredictionResult ImageRecognizer::PredictFromMat(
    const cv::Mat& rawImage)
{
    if (rawImage.empty())
    {
        throw std::runtime_error("Input image is empty");
    }

    cv::Mat image;
    cv::cvtColor(rawImage, image, cv::COLOR_BGR2RGB);
    cv::resize(image, image, cv::Size(input_width, input_height));
    image.convertTo(image, CV_32F, 1.0 / 255.0);

    std::vector<cv::Mat> channels;
    cv::split(image, channels);
    const float means[] = {0.485F, 0.456F, 0.406F};
    const float deviations[] = {0.229F, 0.224F, 0.225F};
    for (size_t i = 0; i < channels.size(); ++i)
    {
        channels[i] = (channels[i] - means[i]) / deviations[i];
    }
    cv::merge(channels, image);

    cv::Mat blob = cv::dnn::blobFromImage(image);
    const std::vector<int64_t> dimensions = {
        1,
        3,
        input_height,
        input_width
    };
    const size_t inputSize =
        static_cast<size_t>(3 * input_height * input_width);
    Ort::MemoryInfo memoryInfo = Ort::MemoryInfo::CreateCpu(
        OrtAllocatorType::OrtArenaAllocator,
        OrtMemType::OrtMemTypeDefault);
    Ort::Value inputTensor = Ort::Value::CreateTensor<float>(
        memoryInfo,
        blob.ptr<float>(),
        inputSize,
        dimensions.data(),
        dimensions.size());

    const char* inputNames[] = {input_name.c_str()};
    const char* outputNames[] = {output_name.c_str()};
    auto outputTensors = session->Run(
        Ort::RunOptions{nullptr},
        inputNames,
        &inputTensor,
        1,
        outputNames,
        1);

    float* output = outputTensors.front().GetTensorMutableData<float>();
    const size_t outputCount = outputTensors.front()
        .GetTensorTypeAndShapeInfo()
        .GetElementCount();
    const size_t classCount = std::min(outputCount, labels.size());
    if (classCount == 0)
    {
        throw std::runtime_error("Image model produced no classes");
    }

    const auto maximum = std::max_element(output, output + classCount);
    const size_t predictedClass =
        static_cast<size_t>(maximum - output);
    const float maximumLogit = *maximum;

    double softmaxDenominator = 0.0;
    for (size_t i = 0; i < classCount; ++i)
    {
        softmaxDenominator += std::exp(
            static_cast<double>(output[i] - maximumLogit));
    }

    return PredictionResult{
        labels[predictedClass],
        static_cast<float>(1.0 / softmaxDenominator)
    };
}
