// Grab.cpp
/*
    Note: Before getting started, Basler recommends reading the "Programmer's Guide" topic
    in the pylon C++ API documentation delivered with pylon.
    If you are upgrading to a higher major version of pylon, Basler also
    strongly recommends reading the "Migrating from Previous Versions" topic in the pylon C++ API documentation.

    This sample illustrates how to grab and process images using the CInstantCamera class.
    The images are grabbed and processed asynchronously, i.e.,
    while the application is processing a buffer, the acquisition of the next buffer is done
    in parallel.

    The CInstantCamera class uses a pool of buffers to retrieve image data
    from the camera device. Once a buffer is filled and ready,
    the buffer can be retrieved from the camera object for processing. The buffer
    and additional image data are collected in a grab result. The grab result is
    held by a smart pointer after retrieval. The buffer is automatically reused
    when explicitly released or when the smart pointer object is destroyed.
*/

// Include files to use the pylon API.
#include <string>
#include <iostream>
#include <thread>
#include <mutex>          // std::mutex
#include <pylon/PylonIncludes.h>
#include <opencv2/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/xfeatures2d.hpp>
#include <opencv2/xfeatures2d/nonfree.hpp>
// Include file to use pylon universal instant camera parameters.
#include <pylon/BaslerUniversalInstantCamera.h>
#include <pylon/BaslerUniversalInstantCameraArray.h>
#include <queue>
#ifdef PYLON_WIN_BUILD
#    include <pylon/PylonGUI.h>
#endif
#include <mutex>          // std::mutex, std::lock
// Namespace for using pylon objects.
using namespace Pylon;

// Namespace for using cout.
using namespace std;
// Namespace for using pylon universal instant camera parameters.
using namespace Basler_UniversalCameraParams;
static const size_t c_maxCamerasToUse = 2;
// Forward declarations for helper functions
bool IsColorCamera( CBaslerUniversalInstantCamera& camera );
void AutoGainOnce( CBaslerUniversalInstantCamera& camera );
void AutoGainContinuous( CBaslerUniversalInstantCamera& camera );
void AutoExposureOnce( CBaslerUniversalInstantCamera& camera );
void AutoExposureContinuous( CBaslerUniversalInstantCamera& camera );
void AutoWhiteBalance( CBaslerUniversalInstantCamera& camera );
std::mutex mtx; 
// Number of images to be grabbed.
static const uint32_t c_countOfImagesToGrab = 30;
//Enumeration used for distinguishing different events.
enum MyEvents
{
    eMyExposureEndEvent = 100,
    eMyEventOverrunEvent = 200
    // More events can be added here.
};
queue<std::pair<cv::Mat,std::string>> integer_queue;
int frame_num = 0;
//Example of an image event handler.
// Example handler for camera events.
class CSampleCameraEventHandler : public CBaslerUniversalCameraEventHandler
{
public:
    // Only very short processing tasks should be performed by this method. Otherwise, the event notification will block the
    // processing of images.
    virtual void OnCameraEvent( CBaslerUniversalInstantCamera& camera, intptr_t userProvidedId, GenApi::INode* /* pNode */ )
    {
        std::cout << std::endl;
        switch (userProvidedId)
        {
            case eMyExposureEndEvent: // Exposure End event
                if (camera.EventExposureEndFrameID.IsReadable()) // Applies to cameras based on SFNC 2.0 or later, e.g, USB cameras
                {
                    cout << "Exposure End event. FrameID: " << camera.EventExposureEndFrameID.GetValue() << " Timestamp: " << camera.EventExposureEndTimestamp.GetValue() << std::endl << std::endl;

                }
                else
                {
                    cout << "Exposure End event. FrameID: " << camera.ExposureEndEventFrameID.GetValue() << " Timestamp: " << camera.ExposureEndEventTimestamp.GetValue() << std::endl << std::endl;
                }
                break;
            case eMyEventOverrunEvent:  // Event Overrun event
                cout << "Event Overrun event. FrameID: " << camera.EventOverrunEventFrameID.GetValue() << " Timestamp: " << camera.EventOverrunEventTimestamp.GetValue() << std::endl << std::endl;
                break;
        }
    }
};
class CSampleImageEventHandler : public CImageEventHandler
{
public:
    virtual void OnImageGrabbed( CInstantCamera& camera, const CGrabResultPtr& ptrGrabResult)
    {
        cv::Mat cameraImg;
        CImageFormatConverter converter;
                    converter.OutputPixelFormat.SetValue(Pylon::PixelType_BGR8packed);
        CPylonImage targetImage;
        cout << "CSampleImageEventHandler::OnImageGrabbed called." << std::endl;
        cout << std::endl;
  
        
    //   mtx.lock();
        if (ptrGrabResult->GrabSucceeded())
            {

         // Create a target image
                converter.Convert( targetImage, ptrGrabResult);
               // intptr_t cameraContextValue = ptrGrabResult->GetCameraContext();
                cameraImg= cv::Mat(ptrGrabResult->GetHeight(),ptrGrabResult->GetWidth(),CV_8UC3,(uint8_t*) targetImage.GetBuffer(),cv::Mat::AUTO_STEP);//,cv::Mat::AUTO_STEP
             //   cout <<"Camera "+ std::to_string(cameraContextValue)+ ": "<< cameras[cameraContextValue].GetDeviceInfo().GetModelName() <<endl;
                // Access the image data
                
                cout << "SizeX: " << ptrGrabResult->GetWidth() << endl;
                cout << "SizeY: " << ptrGrabResult->GetHeight() << endl;
                const uint8_t* pImageBuffer = (uint8_t*) ptrGrabResult->GetBuffer();
                cout << "Gray value of first pixel: " << (uint32_t) pImageBuffer[0] << endl << endl;
                //string windowName = "Live Video: Camera " + std::to_string(cameraContextValue);
           
              string name = string(camera.GetDeviceInfo().GetDeviceIdx());
              
               string windowName = "Live Video: Camera " + name;
                   integer_queue.push(std::make_pair(cameraImg,windowName));
              //  cv::namedWindow(windowName, std::stoi(name));
              
               // cv::imshow(windowName, cameraImg);
               // cv::waitKey(2);
        }
        else
            {
                cout << "Error: " << std::hex << ptrGrabResult->GetErrorCode() << std::dec << " " << ptrGrabResult->GetErrorDescription() << endl;
            }   
            
     
    }
   

};

//class Basler_CameraView
//{
   // public:
   // void operator()(DeviceInfoList_t& device, size_t index)
   void Basler_CameraView(DeviceInfoList_t& device, size_t index)
    {
        PylonInitialize();
        CTlFactory& tlFactory = CTlFactory::GetInstance();
        CBaslerUniversalInstantCamera camera( tlFactory.CreateDevice( device[index] ));
        CGrabResultPtr ptrGrabResult;
        CSampleCameraEventHandler* pHandler1 = new CSampleCameraEventHandler;
        try
        {
                cout << "Using device " << camera.GetDeviceInfo().GetModelName() << endl;

                camera.RegisterImageEventHandler( new CSampleImageEventHandler, RegistrationMode_ReplaceAll, Cleanup_Delete );
   
                camera.GrabCameraEvents = true;

                 if (!camera.EventSelector.IsWritable())
                {
                    throw RUNTIME_EXCEPTION( "The device doesn't support events." );
                }   
                

        camera.Open();
        int minLowerLimit = camera.AutoGainLowerLimit.GetMin();
                int maxUpperLimit = camera.AutoGainUpperLimit.GetMax();
                camera.AutoGainLowerLimit.SetValue(minLowerLimit);
                camera.AutoGainUpperLimit.SetValue(maxUpperLimit);
                camera.AutoFunctionROIUseBrightness.SetValue(true);
                camera.ExposureTime.SetValue(8333);
                camera.GainAuto.SetValue("Continuous");
                camera.AcquisitionFrameRateEnable = true;
                camera.AcquisitionFrameRate = 30;
                camera.UserSetSelector.SetValue(UserSetSelector_Default);
                  camera.UserSetLoad.Execute();
                camera.LineSelector.SetValue(LineSelector_Line4);
                camera.LineMode.SetValue(LineMode_Input);
               camera.TriggerSelector.SetValue(TriggerSelector_FrameStart);
               camera.TriggerSource.SetValue(TriggerSource_Line4);
                camera.TriggerMode.SetValue(TriggerMode_On);
        CImageFormatConverter converter;
                    converter.OutputPixelFormat.SetValue(Pylon::PixelType_BGR8packed);

        // Camera event processing must be activated first, the default is off.
        camera.RegisterCameraEventHandler( pHandler1, "EventExposureEndData", eMyExposureEndEvent, RegistrationMode_ReplaceAll, Cleanup_Delete );        
          
        // For demonstration purposes only, register another image event handler.
         camera.EventSelector.SetValue( EventSelector_ExposureEnd );
        // Enable it.
        if (!camera.EventNotification.TrySetValue( EventNotification_On ))
        {
            // scout-f, scout-g, and aviator GigE cameras use a different value
            camera.EventNotification.SetValue( EventNotification_GenICamEvent );
        }
     
        // Print the model name of the camera
        // This smart pointer will receive the grab result data.
        camera.StartGrabbing(GrabStrategy_OneByOne,GrabLoop_ProvidedByUser);

        while(camera.IsGrabbing())
        {
                int64_t  status = camera.LineStatus.GetValue();
                cout << "Line status: " << status << endl;
                camera.RetrieveResult( 5000, ptrGrabResult, TimeoutHandling_ThrowException );
                ptrGrabResult.Release();
        }

        }
       catch (const GenericException& e)
        {
            // Error handling.
            cerr << "An exception occurred." << endl
            << e.GetDescription() << endl;
         //   exitCode = 1;
        }
        
    // Comment the following two lines to disable waiting on exit.
    delete pHandler1;
    //cerr << endl << "Press enter to exit." << endl;
    //while (cin.get() != '\n');
    PylonTerminate();
    }
     
//};
void show_image(void)
{
    while(1){
               if(integer_queue.size() > 1)
            {

                cv::namedWindow(integer_queue.back().second, cv::WINDOW_NORMAL);
		cv::resizeWindow(integer_queue.back().second,300,700);
                cv::imshow(integer_queue.back().second, integer_queue.back().first);
                cv::waitKey(1);
                integer_queue.pop();

            }
    }
}
int main( int /*argc*/, char* /*argv*/[] )
{
        // Create an instant camera object with the camera device found first.
        DeviceInfoList_t devices;
        CBaslerUniversalInstantCameraArray cameras( min( devices.size(), c_maxCamerasToUse));
        std::vector<std::thread> thread_vec;
        int exitCode = 0;
        
        PylonInitialize();

            CTlFactory& tlFactory = CTlFactory::GetInstance();
            // The exit code of the sample application.
      
            // Before using any pylon methods, the pylon runtime must be initialized.
           if (tlFactory.EnumerateDevices( devices ) == 0)
            {
                throw RUNTIME_EXCEPTION( "No camera present." );
            }

            for (size_t i = 0; i < devices.size(); ++i)
            {
        
                thread_vec.push_back(std::thread(Basler_CameraView, std::ref(devices), i));
                // Print the model name of the camera.
            }
            thread_vec.push_back(std::thread(show_image));
        for(std::thread &thread : thread_vec)
        {
            thread.join();
        }
        
        // Get all attached devices and exit application if no device is found.
            
        /*for(size_t i = 0 ;i < cameras.GetSize(); ++i){
                
                int minLowerLimit = cameras[i].AutoGainLowerLimit.GetMin();
                int maxUpperLimit = cameras[i].AutoGainUpperLimit.GetMax();
                cameras[i].AutoGainLowerLimit.SetValue(minLowerLimit);
                cameras[i].AutoGainUpperLimit.SetValue(maxUpperLimit);
                cameras[i].AutoFunctionROIUseBrightness.SetValue(true);
                cameras[i].ExposureTime.SetValue(8333);
                cameras[i].GainAuto.SetValue("Continuous");
                cameras[i].AcquisitionFrameRateEnable = true;
                cameras[i].AcquisitionFrameRate = 30;
                cameras[i].UserSetSelector = "Default";
                cameras[i].UserSetLoad.Execute();
                cameras[i].LineSelector = "Line4";
                cameras[i].LineMode = "Input";
                cameras[i].TriggerSelector = "FrameStart";
                cameras[i].TriggerSource = "Line4";
                cameras[i].TriggerMode = "On";
                 // For demonstration purposes only, register another image event handler.
                cameras[i].RegisterImageEventHandler( new CSampleImageEventHandler, RegistrationMode_Append, Cleanup_Delete );
                cameras[i].GrabCameraEvents = true;
                cameras[i].Open();
                 if (!cameras[i].EventSelector.IsWritable())
                {
                    throw RUNTIME_EXCEPTION( "The device doesn't support events." );
                }   
                // Print the model name of the camera.
                cout << "Using device " << cameras[i].GetDeviceInfo().GetModelName() <<endl;
        }
        */
       //;
    
    cerr << endl << "Press enter to exit." << endl;
    while (cin.get() != '\n');

    // Releases all pylon resources.
    PylonTerminate();

    return exitCode;
}

void AutoGainOnce( CBaslerUniversalInstantCamera& camera )
{
    // Check whether the gain auto function is available.
    if (!camera.GainAuto.IsWritable())
    {
        cout << "The camera does not support Gain Auto." << endl << endl;
        return;
    }

    // Maximize the grabbed image area of interest (Image AOI).
    camera.OffsetX.TrySetToMinimum();
    camera.OffsetY.TrySetToMinimum();
    camera.Width.SetToMaximum();
    camera.Height.SetToMaximum();

    if (camera.AutoFunctionROISelector.IsWritable()) // Cameras based on SFNC 2.0 or later, e.g., USB cameras
    {
        // Set the Auto Function ROI for luminance statistics.
        // We want to use ROI1 for gathering the statistics

        camera.AutoFunctionROISelector.SetValue( AutoFunctionROISelector_ROI1 );
        camera.AutoFunctionROIUseBrightness.TrySetValue( true );   // ROI 1 is used for brightness control
        camera.AutoFunctionROISelector.SetValue( AutoFunctionROISelector_ROI2 );
        camera.AutoFunctionROIUseBrightness.TrySetValue( false );   // ROI 2 is not used for brightness control

        // Set the ROI (in this example the complete sensor is used)
        camera.AutoFunctionROISelector.SetValue( AutoFunctionROISelector_ROI1 );  // configure ROI 1
        camera.AutoFunctionROIOffsetX.SetToMinimum();
        camera.AutoFunctionROIOffsetY.SetToMinimum();
        camera.AutoFunctionROIWidth.SetToMaximum();
        camera.AutoFunctionROIHeight.SetToMaximum();
    }
    else if (camera.AutoFunctionAOISelector.IsWritable())
    {
        // Set the Auto Function AOI for luminance statistics.
        // Currently, AutoFunctionAOISelector_AOI1 is predefined to gather
        // luminance statistics.
        camera.AutoFunctionAOISelector.SetValue( AutoFunctionAOISelector_AOI1 );
        camera.AutoFunctionAOIOffsetX.SetToMinimum();
        camera.AutoFunctionAOIOffsetY.SetToMinimum();
        camera.AutoFunctionAOIWidth.SetToMaximum();
        camera.AutoFunctionAOIHeight.SetToMaximum();
    }

    if (camera.GetSfncVersion() >= Sfnc_2_0_0) // Cameras based on SFNC 2.0 or later, e.g., USB cameras
    {
        // Set the target value for luminance control.
        // A value of 0.3 means that the target brightness is 30 % of the maximum brightness of the raw pixel value read out from the sensor.
        // A value of 0.4 means 40 % and so forth.
        camera.AutoTargetBrightness.SetValue( 0.3 );

        // We are going to try GainAuto = Once.

        cout << "Trying 'GainAuto = Once'." << endl;
        cout << "Initial Gain = " << camera.Gain.GetValue() << endl;

        // Set the gain ranges for luminance control.
        camera.AutoGainLowerLimit.SetToMinimum();
        camera.AutoGainUpperLimit.SetToMaximum();
    }
    else
    {
        // Set the target value for luminance control. The value is always expressed
        // as an 8 bit value regardless of the current pixel data output format,
        // i.e., 0 -> black, 255 -> white.
        camera.AutoTargetValue.TrySetValue( 80 );

        // We are going to try GainAuto = Once.

        cout << "Trying 'GainAuto = Once'." << endl;
        cout << "Initial Gain = " << camera.GainRaw.GetValue() << endl;

        // Set the gain ranges for luminance control.
        camera.AutoGainRawLowerLimit.SetToMinimum();
        camera.AutoGainRawUpperLimit.SetToMaximum();
    }

    camera.GainAuto.SetValue( GainAuto_Once );

    // When the "once" mode of operation is selected,
    // the parameter values are automatically adjusted until the related image property
    // reaches the target value. After the automatic parameter value adjustment is complete, the auto
    // function will automatically be set to "off" and the new parameter value will be applied to the
    // subsequently grabbed images.

    int n = 0;
    while (camera.GainAuto.GetValue() != GainAuto_Off)
    {
        CBaslerUniversalGrabResultPtr ptrGrabResult;
        camera.GrabOne( 5000, ptrGrabResult );
#ifdef PYLON_WIN_BUILD
        Pylon::DisplayImage( 1, ptrGrabResult );
#endif
        ++n;
        //For demonstration purposes only. Wait until the image is shown.
        WaitObject::Sleep( 100 );

        //Make sure the loop is exited.
        if (n > 100)
        {
            throw TIMEOUT_EXCEPTION( "The adjustment of auto gain did not finish." );
        }
    }

    cout << "GainAuto went back to 'Off' after " << n << " frames." << endl;
    if (camera.Gain.IsReadable()) // Cameras based on SFNC 2.0 or later, e.g., USB cameras
    {
        cout << "Final Gain = " << camera.Gain.GetValue() << endl << endl;
    }
    else
    {
        cout << "Final Gain = " << camera.GainRaw.GetValue() << endl << endl;
    }
}


void AutoGainContinuous( CBaslerUniversalInstantCamera& camera )
{
    // Check whether the Gain Auto feature is available.
    if (!camera.GainAuto.IsWritable())
    {
        cout << "The camera does not support Gain Auto." << endl << endl;
        return;
    }

    // Maximize the grabbed image area of interest (Image AOI).
    camera.OffsetX.TrySetToMinimum();
    camera.OffsetY.TrySetToMinimum();
    camera.Width.SetToMaximum();
    camera.Height.SetToMaximum();

    if (camera.AutoFunctionROISelector.IsWritable()) // Cameras based on SFNC 2.0 or later, e.g., USB cameras
    {
        // Set the Auto Function ROI for luminance statistics.
        // We want to use ROI1 for gathering the statistics

        camera.AutoFunctionROISelector.SetValue( AutoFunctionROISelector_ROI1 );
        camera.AutoFunctionROIUseBrightness.TrySetValue( true );   // ROI 1 is used for brightness control
        camera.AutoFunctionROISelector.SetValue( AutoFunctionROISelector_ROI2 );
        camera.AutoFunctionROIUseBrightness.TrySetValue( false );   // ROI 2 is not used for brightness control


        // Set the ROI (in this example the complete sensor is used)
        camera.AutoFunctionROISelector.SetValue( AutoFunctionROISelector_ROI1 );  // configure ROI 1
        camera.AutoFunctionROIOffsetX.SetToMinimum();
        camera.AutoFunctionROIOffsetY.SetToMinimum();
        camera.AutoFunctionROIWidth.SetToMaximum();
        camera.AutoFunctionROIHeight.SetToMaximum();
    }
    else if (camera.AutoFunctionAOISelector.IsWritable())
    {
        // Set the Auto Function AOI for luminance statistics.
        // Currently, AutoFunctionAOISelector_AOI1 is predefined to gather
        // luminance statistics.
        camera.AutoFunctionAOISelector.SetValue( AutoFunctionAOISelector_AOI1 );
        camera.AutoFunctionAOIOffsetX.SetToMinimum();
        camera.AutoFunctionAOIOffsetY.SetToMinimum();
        camera.AutoFunctionAOIWidth.SetToMaximum();
        camera.AutoFunctionAOIHeight.SetToMaximum();
    }

    if (camera.GetSfncVersion() >= Sfnc_2_0_0) // Cameras based on SFNC 2.0 or later, e.g., USB cameras
    {
        // Set the target value for luminance control.
        // A value of 0.3 means that the target brightness is 30 % of the maximum brightness of the raw pixel value read out from the sensor.
        // A value of 0.4 means 40 % and so forth.
        camera.AutoTargetBrightness.SetValue( 0.3 );

        // We are trying GainAuto = Continuous.
        cout << "Trying 'GainAuto = Continuous'." << endl;
        cout << "Initial Gain = " << camera.Gain.GetValue() << endl;

        camera.GainAuto.SetValue( GainAuto_Continuous );
    }
    else
    {
        // Set the target value for luminance control. The value is always expressed
        // as an 8 bit value regardless of the current pixel data output format,
        // i.e., 0 -> black, 255 -> white.
        camera.AutoTargetValue.TrySetValue( 80 );

        // We are trying GainAuto = Continuous.
        cout << "Trying 'GainAuto = Continuous'." << endl;
        cout << "Initial Gain = " << camera.GainRaw.GetValue() << endl;

        camera.GainAuto.SetValue( GainAuto_Continuous );
    }

    // When "continuous" mode is selected, the parameter value is adjusted repeatedly while images are acquired.
    // Depending on the current frame rate, the automatic adjustments will usually be carried out for
    // every or every other image unless the camera's micro controller is kept busy by other tasks.
    // The repeated automatic adjustment will proceed until the "once" mode of operation is used or
    // until the auto function is set to "off", in which case the parameter value resulting from the latest
    // automatic adjustment will operate unless the value is manually adjusted.
    for (int n = 0; n < 20; n++)            // For demonstration purposes, we will grab "only" 20 images.
    {
        CBaslerUniversalGrabResultPtr ptrGrabResult;
        camera.GrabOne( 5000, ptrGrabResult );
#ifdef PYLON_WIN_BUILD
        Pylon::DisplayImage( 1, ptrGrabResult );
#endif

        //For demonstration purposes only. Wait until the image is shown.
        WaitObject::Sleep( 100 );
    }
    camera.GainAuto.SetValue( GainAuto_Off ); // Switch off GainAuto.

    if (camera.Gain.IsReadable()) // Cameras based on SFNC 2.0 or later, e.g., USB cameras
    {
        cout << "Final Gain = " << camera.Gain.GetValue() << endl << endl;
    }
    else
    {
        cout << "Final Gain = " << camera.GainRaw.GetValue() << endl << endl;
    }
}


void AutoExposureOnce( CBaslerUniversalInstantCamera& camera )
{
    // Check whether auto exposure is available
    if (!camera.ExposureAuto.IsWritable())
    {
        cout << "The camera does not support Exposure Auto." << endl << endl;
        return;
    }

    // Maximize the grabbed area of interest (Image AOI).
    camera.OffsetX.TrySetToMinimum();
    camera.OffsetY.TrySetToMinimum();
    camera.Width.SetToMaximum();
    camera.Height.SetToMaximum();

    if (camera.AutoFunctionROISelector.IsWritable())
    {
        // Set the Auto Function ROI for luminance statistics.
        // We want to use ROI1 for gathering the statistics
        camera.AutoFunctionROISelector.SetValue( AutoFunctionROISelector_ROI1 );
        camera.AutoFunctionROIUseBrightness.TrySetValue( true );   // ROI 1 is used for brightness control
        camera.AutoFunctionROISelector.SetValue( AutoFunctionROISelector_ROI2 );
        camera.AutoFunctionROIUseBrightness.TrySetValue( false );   // ROI 2 is not used for brightness control

        // Set the ROI (in this example the complete sensor is used)
        camera.AutoFunctionROISelector.SetValue( AutoFunctionROISelector_ROI1 );  // configure ROI 1
        camera.AutoFunctionROIOffsetX.SetToMinimum();
        camera.AutoFunctionROIOffsetY.SetToMinimum();
        camera.AutoFunctionROIWidth.SetToMaximum();
        camera.AutoFunctionROIHeight.SetToMaximum();
    }
    else if (camera.AutoFunctionAOISelector.IsWritable())
    {
        // Set the Auto Function AOI for luminance statistics.
        // Currently, AutoFunctionAOISelector_AOI1 is predefined to gather
        // luminance statistics.
        camera.AutoFunctionAOISelector.SetValue( AutoFunctionAOISelector_AOI1 );
        camera.AutoFunctionAOIOffsetX.SetToMinimum();
        camera.AutoFunctionAOIOffsetY.SetToMinimum();
        camera.AutoFunctionAOIWidth.SetToMaximum();
        camera.AutoFunctionAOIHeight.SetToMaximum();
    }

    if (camera.GetSfncVersion() >= Sfnc_2_0_0) // Cameras based on SFNC 2.0 or later, e.g., USB cameras
    {
        // Set the target value for luminance control.
        // A value of 0.3 means that the target brightness is 30 % of the maximum brightness of the raw pixel value read out from the sensor.
        // A value of 0.4 means 40 % and so forth.
        camera.AutoTargetBrightness.SetValue( 0.3 );

        // Try ExposureAuto = Once.
        cout << "Trying 'ExposureAuto = Once'." << endl;
        cout << "Initial exposure time = ";
        cout << camera.ExposureTime.GetValue() << " us" << endl;

        // Set the exposure time ranges for luminance control.
        camera.AutoExposureTimeLowerLimit.SetToMinimum();
        camera.AutoExposureTimeUpperLimit.SetToMaximum();

        camera.ExposureAuto.SetValue( ExposureAuto_Once );
    }
    else
    {
        // Set the target value for luminance control. The value is always expressed
        // as an 8 bit value regardless of the current pixel data output format,
        // i.e., 0 -> black, 255 -> white.
        camera.AutoTargetValue.SetValue( 80 );

        // Try ExposureAuto = Once.
        cout << "Trying 'ExposureAuto = Once'." << endl;
        cout << "Initial exposure time = ";
        cout << camera.ExposureTimeAbs.GetValue() << " us" << endl;

        // Set the exposure time ranges for luminance control.
        camera.AutoExposureTimeAbsLowerLimit.SetToMinimum();
        camera.AutoExposureTimeAbsUpperLimit.SetToMaximum();

        camera.ExposureAuto.SetValue( ExposureAuto_Once );
    }

    // When the "once" mode of operation is selected,
    // the parameter values are automatically adjusted until the related image property
    // reaches the target value. After the automatic parameter value adjustment is complete, the auto
    // function will automatically be set to "off", and the new parameter value will be applied to the
    // subsequently grabbed images.
    int n = 0;
    while (camera.ExposureAuto.GetValue() != ExposureAuto_Off)
    {
        CBaslerUniversalGrabResultPtr ptrGrabResult;
        camera.GrabOne( 5000, ptrGrabResult );
#ifdef PYLON_WIN_BUILD
        Pylon::DisplayImage( 1, ptrGrabResult );
#endif
        ++n;

        //For demonstration purposes only. Wait until the image is shown.
        WaitObject::Sleep( 100 );

        //Make sure the loop is exited.
        if (n > 100)
        {
            throw TIMEOUT_EXCEPTION( "The adjustment of auto exposure did not finish." );
        }
    }
    cout << "ExposureAuto went back to 'Off' after " << n << " frames." << endl;
    cout << "Final exposure time = ";
    if (camera.ExposureTime.IsReadable()) // Cameras based on SFNC 2.0 or later, e.g., USB cameras
    {
        cout << camera.ExposureTime.GetValue() << " us" << endl << endl;
    }
    else
    {
        cout << camera.ExposureTimeAbs.GetValue() << " us" << endl << endl;
    }
}


void AutoExposureContinuous( CBaslerUniversalInstantCamera& camera )
{
    // Check whether the Exposure Auto feature is available.
    if (!camera.ExposureAuto.IsWritable())
    {
        cout << "The camera does not support Exposure Auto." << endl << endl;
        return;
    }

    // Maximize the grabbed area of interest (Image AOI).
    camera.OffsetX.TrySetToMinimum();
    camera.OffsetY.TrySetToMinimum();
    camera.Width.SetToMaximum();
    camera.Height.SetToMaximum();

    if (camera.AutoFunctionROISelector.IsWritable()) // Cameras based on SFNC 2.0 or later, e.g., USB cameras
    {
        // Set the Auto Function ROI for luminance statistics.
        // We want to use ROI1 for gathering the statistics

        camera.AutoFunctionROISelector.SetValue( AutoFunctionROISelector_ROI1 );
        camera.AutoFunctionROIUseBrightness.TrySetValue( true );   // ROI 1 is used for brightness control
        camera.AutoFunctionROISelector.SetValue( AutoFunctionROISelector_ROI2 );
        camera.AutoFunctionROIUseBrightness.TrySetValue( false );   // ROI 2 is not used for brightness control

        // Set the ROI (in this example the complete sensor is used)
        camera.AutoFunctionROISelector.SetValue( AutoFunctionROISelector_ROI1 );  // configure ROI 1
        camera.AutoFunctionROIOffsetX.SetToMinimum();
        camera.AutoFunctionROIOffsetY.SetToMinimum();
        camera.AutoFunctionROIWidth.SetToMaximum();
        camera.AutoFunctionROIHeight.SetToMaximum();
    }
    else if (camera.AutoFunctionAOISelector.IsWritable())
    {
        // Set the Auto Function AOI for luminance statistics.
        // Currently, AutoFunctionAOISelector_AOI1 is predefined to gather
        // luminance statistics.
        camera.AutoFunctionAOISelector.SetValue( AutoFunctionAOISelector_AOI1 );
        camera.AutoFunctionAOIOffsetX.SetToMinimum();
        camera.AutoFunctionAOIOffsetY.SetToMinimum();
        camera.AutoFunctionAOIWidth.SetToMaximum();
        camera.AutoFunctionAOIHeight.SetToMaximum();
    }

    if (camera.GetSfncVersion() >= Sfnc_2_0_0) // Cameras based on SFNC 2.0 or later, e.g., USB cameras
    {
        // Set the target value for luminance control.
        // A value of 0.3 means that the target brightness is 30 % of the maximum brightness of the raw pixel value read out from the sensor.
        // A value of 0.4 means 40 % and so forth.
        camera.AutoTargetBrightness.SetValue( 0.3 );

        cout << "ExposureAuto 'GainAuto = Continuous'." << endl;
        cout << "Initial exposure time = ";
        cout << camera.ExposureTime.GetValue() << " us" << endl;

        camera.ExposureAuto.SetValue( ExposureAuto_Continuous );
    }
    else
    {
        // Set the target value for luminance control. The value is always expressed
        // as an 8 bit value regardless of the current pixel data output format,
        // i.e., 0 -> black, 255 -> white.
        camera.AutoTargetValue.SetValue( 80 );

        cout << "ExposureAuto 'GainAuto = Continuous'." << endl;
        cout << "Initial exposure time = ";
        cout << camera.ExposureTimeAbs.GetValue() << " us" << endl;

        camera.ExposureAuto.SetValue( ExposureAuto_Continuous );
    }

    // When "continuous" mode is selected, the parameter value is adjusted repeatedly while images are acquired.
    // Depending on the current frame rate, the automatic adjustments will usually be carried out for
    // every or every other image, unless the camera's microcontroller is kept busy by other tasks.
    // The repeated automatic adjustment will proceed until the "once" mode of operation is used or
    // until the auto function is set to "off", in which case the parameter value resulting from the latest
    // automatic adjustment will operate unless the value is manually adjusted.
    for (int n = 0; n < 20; n++)    // For demonstration purposes, we will use only 20 images.
    {
        CBaslerUniversalGrabResultPtr ptrGrabResult;
        camera.GrabOne( 5000, ptrGrabResult );
#ifdef PYLON_WIN_BUILD
        Pylon::DisplayImage( 1, ptrGrabResult );
#endif

        //For demonstration purposes only. Wait until the image is shown.
        WaitObject::Sleep( 100 );
    }
    camera.ExposureAuto.SetValue( ExposureAuto_Off ); // Switch off Exposure Auto.

    cout << "Final exposure time = ";
    if (camera.ExposureTime.IsReadable()) // Cameras based on SFNC 2.0 or later, e.g., USB cameras
    {
        cout << camera.ExposureTime.GetValue() << " us" << endl << endl;
    }
    else
    {
        cout << camera.ExposureTimeAbs.GetValue() << " us" << endl << endl;
    }
}


void AutoWhiteBalance( CBaslerUniversalInstantCamera& camera )
{
    // Check whether the Balance White Auto feature is available.
    if (!camera.BalanceWhiteAuto.IsWritable())
    {
        cout << "The camera does not support Balance White Auto." << endl << endl;
        return;
    }

    // Maximize the grabbed area of interest (Image AOI).
    camera.OffsetX.TrySetToMinimum();
    camera.OffsetY.TrySetToMinimum();
    camera.Width.SetToMaximum();
    camera.Height.SetToMaximum();

    if (camera.AutoFunctionROISelector.IsWritable()) // Cameras based on SFNC 2.0 or later, e.g., USB cameras
    {
        // Set the Auto Function ROI for white balance.
        // We want to use ROI2
        camera.AutoFunctionROISelector.SetValue( AutoFunctionROISelector_ROI1 );
        camera.AutoFunctionROIUseWhiteBalance.SetValue( false );   // ROI 1 is not used for white balance
        camera.AutoFunctionROISelector.SetValue( AutoFunctionROISelector_ROI2 );
        camera.AutoFunctionROIUseWhiteBalance.SetValue( true );   // ROI 2 is used for white balance

        // Set the Auto Function AOI for white balance statistics.
        // Currently, AutoFunctionROISelector_ROI2 is predefined to gather
        // white balance statistics.
        camera.AutoFunctionROISelector.SetValue( AutoFunctionROISelector_ROI2 );
        camera.AutoFunctionROIOffsetX.SetToMinimum();
        camera.AutoFunctionROIOffsetY.SetToMinimum();
        camera.AutoFunctionROIWidth.SetToMaximum();
        camera.AutoFunctionROIHeight.SetToMaximum();
    }
    else if (camera.AutoFunctionAOISelector.IsWritable())
    {
        // Set the Auto Function AOI for luminance statistics.
        // Currently, AutoFunctionAOISelector_AOI1 is predefined to gather
        // luminance statistics.
        camera.AutoFunctionAOISelector.SetValue( AutoFunctionAOISelector_AOI1 );
        camera.AutoFunctionAOIOffsetX.SetToMinimum();
        camera.AutoFunctionAOIOffsetY.SetToMinimum();
        camera.AutoFunctionAOIWidth.SetToMaximum();
        camera.AutoFunctionAOIHeight.SetToMaximum();
    }

    cout << "Trying 'BalanceWhiteAuto = Once'." << endl;
    cout << "Initial balance ratio: ";

    if (camera.GetSfncVersion() >= Sfnc_2_0_0) // Cameras based on SFNC 2.0 or later, e.g., USB cameras
    {
        camera.BalanceRatioSelector.SetValue( BalanceRatioSelector_Red );
        cout << "R = " << camera.BalanceRatio.GetValue() << "   ";
        camera.BalanceRatioSelector.SetValue( BalanceRatioSelector_Green );
        cout << "G = " << camera.BalanceRatio.GetValue() << "   ";
        camera.BalanceRatioSelector.SetValue( BalanceRatioSelector_Blue );
        cout << "B = " << camera.BalanceRatio.GetValue() << endl;
    }
    else
    {
        camera.BalanceRatioSelector.SetValue( BalanceRatioSelector_Red );
        cout << "R = " << camera.BalanceRatioAbs.GetValue() << "   ";
        camera.BalanceRatioSelector.SetValue( BalanceRatioSelector_Green );
        cout << "G = " << camera.BalanceRatioAbs.GetValue() << "   ";
        camera.BalanceRatioSelector.SetValue( BalanceRatioSelector_Blue );
        cout << "B = " << camera.BalanceRatioAbs.GetValue() << endl;
    }

    camera.BalanceWhiteAuto.SetValue( BalanceWhiteAuto_Once );

    // When the "once" mode of operation is selected,
    // the parameter values are automatically adjusted until the related image property
    // reaches the target value. After the automatic parameter value adjustment is complete, the auto
    // function will automatically be set to "off" and the new parameter value will be applied to the
    // subsequently grabbed images.
    int n = 0;
    while (camera.BalanceWhiteAuto.GetValue() != BalanceWhiteAuto_Off)
    {
        CBaslerUniversalGrabResultPtr ptrGrabResult;
        camera.GrabOne( 5000, ptrGrabResult );
#ifdef PYLON_WIN_BUILD
        Pylon::DisplayImage( 1, ptrGrabResult );
#endif
        ++n;

        //For demonstration purposes only. Wait until the image is shown.
        WaitObject::Sleep( 100 );

        //Make sure the loop is exited.
        if (n > 100)
        {
            throw TIMEOUT_EXCEPTION( "The adjustment of auto white balance did not finish." );
        }
    }
    cout << "BalanceWhiteAuto went back to 'Off' after ";
    cout << n << " frames." << endl;
    cout << "Final balance ratio: ";

    if (camera.GetSfncVersion() >= Sfnc_2_0_0) // Cameras based on SFNC 2.0 or later, e.g., USB cameras
    {
        camera.BalanceRatioSelector.SetValue( BalanceRatioSelector_Red );
        cout << "R = " << camera.BalanceRatio.GetValue() << "   ";
        camera.BalanceRatioSelector.SetValue( BalanceRatioSelector_Green );
        cout << "G = " << camera.BalanceRatio.GetValue() << "   ";
        camera.BalanceRatioSelector.SetValue( BalanceRatioSelector_Blue );
        cout << "B = " << camera.BalanceRatio.GetValue() << endl;
    }
    else
    {
        camera.BalanceRatioSelector.SetValue( BalanceRatioSelector_Red );
        cout << "R = " << camera.BalanceRatioAbs.GetValue() << "   ";
        camera.BalanceRatioSelector.SetValue( BalanceRatioSelector_Green );
        cout << "G = " << camera.BalanceRatioAbs.GetValue() << "   ";
        camera.BalanceRatioSelector.SetValue( BalanceRatioSelector_Blue );
        cout << "B = " << camera.BalanceRatioAbs.GetValue() << endl;
    }
}


bool IsColorCamera( CBaslerUniversalInstantCamera& camera )
{
    StringList_t settableValues;
    camera.PixelFormat.GetSettableValues( settableValues );
    bool result = false;

    for (size_t i = 0; i < settableValues.size(); i++)
    {
        if (settableValues[i].find( String_t( "Bayer" ) ) != String_t::_npos())
        {
            result = true;
            break;
        }
    }
    return result;
}

/*
 // Create an instant camera object with the camera device found first.
        //CInstantCamera camera( CTlFactory::GetInstance().CreateFirstDevice() );
        //camera = pylon.InstantCamera(pylon.TlFactory.GetInstance().CreateFirstDevice())
        //CBaslerUniversalInstantCamera
        CTlFactory& tlFactory = CTlFactory::GetInstance();
         // Get all attached devices and exit application if no device is found.
        DeviceInfoList_t devices;
        
        if (tlFactory.EnumerateDevices( devices ) == 0)
        {
            throw RUNTIME_EXCEPTION( "No camera present." );
        }
        //Pylon::CBaslerUniversalInstantCamera cameras( min( devices.size(), c_maxCamerasToUse ) );
        CBaslerUniversalInstantCameraArray cameras( min( devices.size(), c_maxCamerasToUse));

         for (size_t i = 0; i < cameras.GetSize(); ++i)
        {
            cameras[i].Attach( tlFactory.CreateDevice( devices[i] ) );
            
            // Print the model name of the camera.
            cout << "Using device " << cameras[i].GetDeviceInfo().GetModelName() << endl;
        }
        CImageFormatConverter converter;
        //camera.RegisterConfiguration( new CAcquireSingleFrameConfiguration, RegistrationMode_ReplaceAll, Cleanup_Delete );
        // Get a camera nodemap in order to access camera parameters.
        // Open the camera.
        //camera.Open();
        // Second the converter must be parameterized.


        // The parameter MaxNumBuffer can be used to control the count of buffers
        // allocated for grabbing. The default value of this parameter is 10.
        //camera.MaxNumBuffer = 5;

        // Start the grabbing of c_countOfImagesToGrab images.
        // The camera device is parameterized with a default configuration which
        // sets up free-running continuous acquisition.
        for (size_t i = 0; i < cameras.GetSize(); ++i){
            cameras[i].Open();
            int minLowerLimit = cameras[i].AutoGainLowerLimit.GetMin();
            int maxUpperLimit = cameras[i].AutoGainUpperLimit.GetMax();
            cameras[i].AutoGainLowerLimit.SetValue(minLowerLimit);
            cameras[i].AutoGainUpperLimit.SetValue(maxUpperLimit);
            cameras[i].AutoFunctionROIUseBrightness.SetValue(true);
            cameras[i].ExposureTime.SetValue(99900);
            cameras[i].GainAuto.SetValue("Continuous");
        }
        

        //converter.OutputBitAlignment = Pylon::OutputBitAlignment_MsbAligned;
        //AutoGainContinuous( camera );
        //AutoExposureContinuous( camera );
        // This smart pointer will receive the grab result data.
        CPylonImage targetImage;
        CGrabResultPtr ptrGrabResult;
        cv::Mat cameraImg;
        converter.OutputPixelFormat = Pylon::PixelType_BGR8packed;

        // This smart pointer will receive the grab result data.
        cameras.StartGrabbing(GrabStrategy_LatestImageOnly);
        // Camera.StopGrabbing() is called automatically by the RetrieveResult() method
        // when c_countOfImagesToGrab images have been retrieved.
        while(cameras.IsGrabbing())
        {
            // Wait for an image and then retrieve it. A timeout of 5000 ms is used.
            cameras.RetrieveResult( 5000, ptrGrabResult, TimeoutHandling_ThrowException );
            // Image grabbed successfully?
            if (ptrGrabResult->GrabSucceeded())
            {
                intptr_t cameraContextValue = ptrGrabResult->GetCameraContext();
                 // Create a target image
                converter.Convert( targetImage, ptrGrabResult);
                cameraImg= cv::Mat(ptrGrabResult->GetHeight(),ptrGrabResult->GetWidth(),CV_8UC3,(uint8_t*) targetImage.GetBuffer(),cv::Mat::AUTO_STEP);//,cv::Mat::AUTO_STEP
                
                // Access the image data
                
                //cout << "SizeX: " << ptrGrabResult->GetWidth() << endl;
                //cout << "SizeY: " << ptrGrabResult->GetHeight() << endl;
                //const uint8_t* pImageBuffer = (uint8_t*) ptrGrabResult->GetBuffer();
                //cout << "Gray value of first pixel: " << (uint32_t) pImageBuffer[0] << endl << endl;
                string windowName = "Live Video";
                cv::namedWindow(windowName, 2);
                cv::imshow(windowName, cameraImg);
                ptrGrabResult.Release();
                cv::waitKey(1);
#ifdef PYLON_WIN_BUILD
                // Display the grabbed image.
                Pylon::DisplayImage( 1, ptrGrabResult );
#endif
            }
            else
            {
                cout << "Error: " << std::hex << ptrGrabResult->GetErrorCode() << std::dec << " " << ptrGrabResult->GetErrorDescription() << endl;
            }
        }
        */
