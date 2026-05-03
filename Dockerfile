FROM node:20-alpine

RUN apk update && apk add --no-cache \
    build-base \
    cmake \
    git \
    asio-dev

RUN git clone https://github.com/CrowCpp/Crow.git /tmp/crow && \
    cd /tmp/crow && \
    mkdir build && cd build && \
    cmake .. -DCROW_BUILD_EXAMPLES=OFF -DCROW_BUILD_TESTS=OFF && \
    make install && \
    rm -rf /tmp/crow

WORKDIR /fyp2026

COPY . .
RUN cmake -B build
RUN cmake --build build

EXPOSE 3000
CMD ["./FYP2026"]

# COPY package.json ./
# RUN npm install
# COPY . .

# CMD ["npm", "start"]