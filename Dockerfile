FROM python

RUN apt-get update
RUN apt-get -y install gdb cmake cppcheck

RUN apt-get install -y flex bison gperf cmake ninja-build ccache libffi-dev libssl-dev dfu-util libusb-1.0-0

WORKDIR /root/esp
RUN git clone -b v5.0.2 --recursive https://github.com/espressif/esp-idf.git

WORKDIR /root/esp/esp-idf
RUN ./install.sh esp32
RUN echo ". /root/esp/esp-idf/export.sh" >> /root/.bashrc

RUN pip install esp_idf_monitor

WORKDIR /workspace
