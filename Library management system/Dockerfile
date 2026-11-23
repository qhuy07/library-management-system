FROM gcc:13

WORKDIR /app

# Tai thu vien HTTP single-header cho C++
RUN curl -sSL https://raw.githubusercontent.com/yhirose/cpp-httplib/master/httplib.h -o httplib.h

COPY Model.h Controller.h web_server.cpp ./
COPY public ./public

RUN g++ -std=c++17 -O2 web_server.cpp -o diamond_web -pthread

EXPOSE 8080
CMD ["./diamond_web"]
