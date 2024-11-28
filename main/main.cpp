// Copyright 2024 Rik Essenius
// 
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
// except in compliance with the License. You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software distributed under the License
// is distributed on an "AS IS" BASIS WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and limitations under the License.

#include "pub_sub.hpp"
#include <nvs_flash.h>
#include <variant>
#include <EllipseFit.h>
#include <CartesianEllipse.h>
#include <MathUtils.h>
#include <iostream> // for std::cout; remove later
#include "MagnetoSensorHmc.hpp"

pubsub::SubscriberCallback callback = std::make_shared<std::function<void(const pubsub::Topic, const pubsub::Payload&)>>(
    [](const pubsub::Topic& topic, const pubsub::Payload& payload) {
        std::visit([&](auto&& arg) {
            std::cout << "Topic: " << topic << ", Payload: " << arg << std::endl;
        }, payload);
    }
);


extern "C" void app_main() {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    printf("Hello world from %ld!\n", __cplusplus);	
    printf("Epsilon is %f\n", EllipseMath::Epsilon);
    printf("M_PI is %f\n", M_PI);
    printf("3^2=%f\n", EllipseMath::sqr(3));

        // start the fitter
    EllipseMath::EllipseFit ellipseFit;
    ellipseFit.begin();

    // create test data generator
    constexpr auto Center = EllipseMath::Coordinate{ 1, 2 };
    constexpr auto Radius = EllipseMath::Coordinate{ 10, 6 };
    constexpr auto XAxisAngle = EllipseMath::Angle{ M_PI / 3 };
    const auto inputEllipse = EllipseMath::CartesianEllipse(Center, Radius, XAxisAngle);
    const unsigned int pointsOnEllipse = EllipseMath::EllipseFit::getSize();

    // feed the fitter with half an ellipse
    double currentAngle = 0;
    const double delta = M_PI / pointsOnEllipse;
    for (unsigned int i = 0; i < pointsOnEllipse; i++) {
        EllipseMath::Coordinate point = inputEllipse.getPointOnEllipseAtAngle(EllipseMath::Angle{ currentAngle });
        ellipseFit.addMeasurement(point);
        currentAngle += delta;
    }

    // execute the fitting
    if (!ellipseFit.bufferIsFull()) {
        printf(" Expected buffer to be full, but it wasn't\n");
    }
    else {
        const auto resultQuadraticEllipse = ellipseFit.fit();

        // Result should be equal to the parameters of the test data generator
        const auto result = EllipseMath::CartesianEllipse(resultQuadraticEllipse);
        printf(" Result:\n   Center (%f, %f)\n   Radius (%f, %f)\n   Angle %f\n",
            result.getCenter().x, result.getCenter().y,
            result.getRadius().x, result.getRadius().y,
            result.getAngle().value);
        // Result:
        //   Center (1.000000, 2.000000)
        //   Radius (10.000000, 6.000000)
        //   Angle 1.047198

        pubsub::PubSub pubsub;
        pubsub.subscribe(1, callback);
        pubsub.publish(1, static_cast<float>(result.getCenter().x));
        pubsub.publish(1, static_cast<float>(result.getCenter().y));
    }
}