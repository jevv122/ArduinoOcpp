// matth-x/ArduinoOcpp
// Copyright Matthias Akstaller 2019 - 2022
// MIT License

#include <Variants.h>

#include <ArduinoOcpp/Tasks/ChargePointStatus/ChargePointStatusService.h>
#include <ArduinoOcpp/Core/OcppEngine.h>
#include <ArduinoOcpp/SimpleOcppOperationFactory.h>

#include <string.h>

using namespace ArduinoOcpp;

ChargePointStatusService::ChargePointStatusService(OcppEngine& context, int numConn)
      : context(context), numConnectors{numConn} {
  
    connectors = (ConnectorStatus**) malloc(numConn * sizeof(ConnectorStatus*));
    for (int i = 0; i < numConn; i++) {
        connectors[i] = new ConnectorStatus(context.getOcppModel(), i);
    }
}

ChargePointStatusService::~ChargePointStatusService() {
    for (int i = 0; i < numConnectors; i++) {
        delete connectors[i];
    }
    free(connectors);
}

void ChargePointStatusService::loop() {
    if (!booted) return;
    for (int i = 0; i < numConnectors; i++){
        auto statusNotificationMsg = connectors[i]->loop();
        if (statusNotificationMsg != nullptr) {
            auto statusNotification = makeOcppOperation(statusNotificationMsg);
            context.initiateOperation(std::move(statusNotification));
        }
    }
}

ConnectorStatus *ChargePointStatusService::getConnector(int connectorId) {
    if (connectorId < 0 || connectorId >= numConnectors) {
        Serial.print(F("[ChargePointStatusService] Error in getConnector(connectorId): connectorId is out of bounds\n"));
        return nullptr;
    }

    return connectors[connectorId];
}

void ChargePointStatusService::authorize(String &idTag){
    this->idTag = String(idTag);
    authorize();
}

void ChargePointStatusService::authorize(){
    if (authorized == true){
        if (DEBUG_OUT) Serial.print(F("[ChargePointStatusService] Warning: authorized twice or didn't unauthorize before\n"));
    }
    authorized = true;
}

void ChargePointStatusService::boot() {
    booted = true;
}

bool ChargePointStatusService::isBooted() {
    return booted;
}

String &ChargePointStatusService::getUnboundIdTag() {
    return idTag;
}

void ChargePointStatusService::invalidateUnboundIdTag() {
    authorized = false;
    idTag = String('\0');
}

boolean ChargePointStatusService::existsUnboundAuthorization() {
    return authorized;
}

void ChargePointStatusService::bindAuthorization(int connectorId) {
    if (connectorId < 0 || connectorId >= numConnectors) {
        Serial.print(F("[ChargePointStatusService] Error in bindAuthorization(connectorId): connectorId is out of bounds\n"));
        return;
    }
    if (!authorized) {
        if (DEBUG_OUT) Serial.print(F("[ChargePointStatusService] Authorize connector though there is no unbound ID tag\n"));
    }
    if (DEBUG_OUT) Serial.print(F("[ChargePointStatusService] Connector "));
    if (DEBUG_OUT) Serial.print(connectorId);
    if (DEBUG_OUT) Serial.print(F(" occupies idTag "));
    if (DEBUG_OUT) Serial.print(idTag);
    if (DEBUG_OUT) Serial.print(F("\n"));

    connectors[connectorId]->authorize(idTag);

    authorized = false;
}

int ChargePointStatusService::getNumConnectors() {
    return numConnectors;
}
