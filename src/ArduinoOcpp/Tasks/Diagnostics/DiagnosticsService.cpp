// matth-x/ArduinoOcpp
// Copyright Matthias Akstaller 2019 - 2022
// MIT License

#include <ArduinoOcpp/Tasks/Diagnostics/DiagnosticsService.h>
#include <ArduinoOcpp/Core/OcppEngine.h>
#include <ArduinoOcpp/Core/OcppModel.h>
#include <ArduinoOcpp/SimpleOcppOperationFactory.h>

#include <ArduinoOcpp/MessagesV16/DiagnosticsStatusNotification.h>

#include <Variants.h>

using namespace ArduinoOcpp;
using Ocpp16::DiagnosticsStatus;

void DiagnosticsService::loop() {
    auto notification = getDiagnosticsStatusNotification();
    if (notification) {
        context.initiateOperation(std::move(notification));
    }

    const auto& timestampNow = context.getOcppModel().getOcppTime().getOcppTimestampNow();
    if (retries > 0 && timestampNow >= nextTry) {

        if (!uploadIssued) {
            if (onUpload != nullptr) {
                if (DEBUG_OUT) Serial.println(F("[DiagnosticsService] call onUpload"));
                onUpload(location, startTime, stopTime);
                uploadIssued = true;
            } else {
                Serial.println(F("[DiagnosticsService] onUpload must be set! (see setOnUpload). Will abort"));
                retries = 0;
                uploadIssued = false;
            }
        }

        if (uploadIssued) {
            if (uploadStatusSampler != nullptr && uploadStatusSampler() == UploadStatus::Uploaded) {
                //success!
                if (DEBUG_OUT) Serial.println(F("[DiagnosticsService] end update routine (by status)"));
                uploadIssued = false;
                retries = 0;
            }

            //check if maximum time elapsed or failed
            const int UPLOAD_TIMEOUT = 60;
            if (timestampNow - nextTry >= UPLOAD_TIMEOUT
                    || (uploadStatusSampler != nullptr && uploadStatusSampler() == UploadStatus::UploadFailed)) {
                //maximum upload time elapsed or failed

                if (uploadStatusSampler == nullptr) {
                    //No way to find out if failed. But maximum time has elapsed. Assume success
                    if (DEBUG_OUT) Serial.println(F("[DiagnosticsService] end update routine (by timer)"));
                    uploadIssued = false;
                    retries = 0;
                } else {
                    //either we have UploadFailed status or (NotDownloaded + timeout) here
                    Serial.println(F("[DiagnosticsService] Upload timeout or failed!"));
                    const int TRANSITION_DELAY = 10;
                    if (retryInterval <= UPLOAD_TIMEOUT + TRANSITION_DELAY) {
                        nextTry = timestampNow;
                        nextTry += TRANSITION_DELAY; //wait for another 10 seconds
                    } else {
                        nextTry += retryInterval;
                    }
                    retries--;
                }
            }
        } //end if (uploadIssued)
    } //end try upload
}

//timestamps before year 2021 will be treated as "undefined"
String DiagnosticsService::requestDiagnosticsUpload(String &location, int retries, unsigned int retryInterval, OcppTimestamp startTime, OcppTimestamp stopTime) {
    if (onUpload == nullptr) //maybe add further plausibility checks
        return String('\0');
    
    this->location = location;
    this->retries = retries;
    this->retryInterval = retryInterval;
    this->startTime = startTime;
    
    OcppTimestamp stopMin = OcppTimestamp(2021,0,0,0,0,0);
    if (stopTime >= stopMin) {
        this->stopTime = stopTime;
    } else {
        auto newStop = context.getOcppModel().getOcppTime().getOcppTimestampNow();
        newStop += 3600 * 24 * 365; //set new stop time one year in future
        this->stopTime = newStop;
    }
    
    if (DEBUG_OUT) {
        Serial.print(F("[DiagnosticsService] Scheduled Diagnostics upload!\n"));
        Serial.print(F("                     location = "));
        Serial.println(this->location);
        Serial.print(F("                     retries = "));
        Serial.print(this->retries);
        Serial.print(F(", retryInterval = "));
        Serial.println(this->retryInterval);
        Serial.print(F("                     startTime = "));
        char dbuf [JSONDATE_LENGTH + 1] = {'\0'};
        this->startTime.toJsonString(dbuf, JSONDATE_LENGTH + 1);
        Serial.println(dbuf);
        Serial.print(F("                     stopTime = "));
        this->stopTime.toJsonString(dbuf, JSONDATE_LENGTH + 1);
        Serial.println(dbuf);
    }

    nextTry = context.getOcppModel().getOcppTime().getOcppTimestampNow();
    nextTry += 5; //wait for 5s before upload
    uploadIssued = false;

    if (DEBUG_OUT) {
        Serial.print(F("[DiagnosticsService] initial try at "));
        char dbuf [JSONDATE_LENGTH + 1] = {'\0'};
        nextTry.toJsonString(dbuf, JSONDATE_LENGTH + 1);
        Serial.println(dbuf);
    }

    return "diagnostics.log";
}


DiagnosticsStatus DiagnosticsService::getDiagnosticsStatus() {
    if (uploadIssued) {
        if (uploadStatusSampler != nullptr) {
            switch (uploadStatusSampler()) {
                case UploadStatus::NotUploaded:
                    return DiagnosticsStatus::Uploading;
                case UploadStatus::Uploaded:
                    return DiagnosticsStatus::Uploaded;
                case UploadStatus::UploadFailed:
                    return DiagnosticsStatus::UploadFailed;
            }
        }
        return DiagnosticsStatus::Uploading;
    }
    return DiagnosticsStatus::Idle;
}

std::unique_ptr<OcppOperation> DiagnosticsService::getDiagnosticsStatusNotification() {

    if (getDiagnosticsStatus() != lastReportedStatus) {
        lastReportedStatus = getDiagnosticsStatus();
        if (lastReportedStatus != DiagnosticsStatus::Idle) {
            OcppMessage *diagNotificationMsg = new Ocpp16::DiagnosticsStatusNotification(lastReportedStatus);
            auto diagNotification = makeOcppOperation(diagNotificationMsg);
            return diagNotification;
        }
    }

    return nullptr;
}

void DiagnosticsService::setOnUpload(std::function<bool(String &location, OcppTimestamp &startTime, OcppTimestamp &stopTime)> onUpload) {
    this->onUpload = onUpload;
}

void DiagnosticsService::setOnUploadStatusSampler(std::function<UploadStatus()> uploadStatusSampler) {
    this->uploadStatusSampler = uploadStatusSampler;
}

#if !defined(AO_CUSTOM_DIAGNOSTICS) && !defined(AO_CUSTOM_WS)
#if defined(ESP32) || defined(ESP8266)

DiagnosticsService *EspWiFi::makeDiagnosticsService(OcppEngine& context) {
    auto diagService = new DiagnosticsService(context);

    /*
     * add onUpload and uploadStatusSampler when logging was implemented
     */

    return diagService;
}

#endif //defined(ESP32) || defined(ESP8266)
#endif //!defined(AO_CUSTOM_UPDATER) && !defined(AO_CUSTOM_WS)
