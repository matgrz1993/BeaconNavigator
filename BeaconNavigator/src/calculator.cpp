#include "calculator.h"
#include "logger.h"
#include <QtAlgorithms>
#include <QtMath>

QList<DistanceToBeacon> Calculator::m_distances;
DistanceToBeacon Calculator::m_last_distance;

matrix Calculator::c_matrix;
matrix Calculator::cat_matrix;
matrix Calculator::b_matrix;

QList<Point> Calculator::m_last_points;
QMap<QString, QList<qint16>> Calculator::m_median_cache;

double Calculator::calcDistance(const qint16& rssi){
    Logger::logMessage("Calculator::calcDistance");
    double A, t, y0;
    if(rssi == 65){
        return 1;
    }
    if(rssi > -65){
        A = 20.03;
        t = -0.766;
        y0 = -70.43;
    } else {
        A = -19.8;
        t = 7.806;
        y0 = -42.45;
    }
    double tmp = ((double)rssi - y0) / A;
    double path = log(tmp) * t;
    if(path < 0){
        path = 0.01;
    }
    return path;
}

bool Calculator::calcMultilateration(Point &position, QList<DistanceToBeacon> distances){
    Logger::logMessage("Calculator::calcMultilateration");
    if(distances.length() < 3){
        Logger::logMessage("ERROR: Less than three beacon's registered!");
        return false;
    }
    int num_of_dist_above_zero = 0;
    for(DistanceToBeacon dtb : distances){
        if(dtb.distance > 0){
            num_of_dist_above_zero++;
            if(num_of_dist_above_zero > 2){
                break;
            }
        }
    }
    if(num_of_dist_above_zero < 3){
        Logger::logMessage("ERROR Less than three beacon's founded!");
        return false;
    }
    m_distances = distances;
    m_last_distance = m_distances.last();
    calcCMatrix();
    calcCATMatrix();
    calcBMatrix();
    return calcPosition(position);
}

bool Calculator::calcWeightedArithMean(Point& position, QList<DistanceToBeacon> distances){
    Logger::logMessage("Calculator::calcWeightedArithMean");
    m_distances = distances;
    Point point{0,0};
    double sum_of_distances = 0;
    for(DistanceToBeacon distance: m_distances){
        point.x += distance.point.x*distance.distance;
        point.y += distance.point.y*distance.distance;
        sum_of_distances += distance.distance;
    }
    point.x /= sum_of_distances;
    point.y /= sum_of_distances;
    position = point;
    return true;
}

void Calculator::calcCMatrix(){
    double a = 0, b = 0, d = 0;
    Point last_point = m_last_distance.point;
    for(DistanceToBeacon distance: m_distances){
        Point point = distance.point;
        a += (point.x - last_point.x)*(point.x - last_point.x);
        b += (point.x - last_point.x)*(point.y - last_point.y);
        d += (point.y - last_point.y)*(point.y - last_point.y);
    }
    a *= 4;
    b *= 4;
    d *= 4;
    double diff = a*d - b*b;
    c_matrix.clear();
    c_matrix.append(d/diff);
    c_matrix.append(-b/diff);
    c_matrix.append(a/diff);
}

void Calculator::calcCATMatrix(){
    cat_matrix.clear();
    Point last_point = m_last_distance.point;
    for(DistanceToBeacon distance: m_distances){
        Point point = distance.point;
        cat_matrix.append(c_matrix[0] * 2 * (point.x - last_point.x) + c_matrix[1] * 2 * (point.y - last_point.y));
    }
    for(DistanceToBeacon distance: m_distances){
        Point point = distance.point;
        cat_matrix.append(c_matrix[1] * 2 * (point.x - last_point.x) + c_matrix[2] * 2 * (point.y - last_point.y));
    }
}

void Calculator::calcBMatrix(){
    b_matrix.clear();
    Point last_point = m_last_distance.point;
    for(DistanceToBeacon distance: m_distances){
        Point point = distance.point;
        b_matrix.append(point.x*point.x - last_point.x*last_point.x
                        + point.y*point.y - last_point.y*last_point.y
                        + m_last_distance.distance*m_last_distance.distance
                        - distance.distance*distance.distance);
    }
}

bool Calculator::calcPosition(Point& position){
    Logger::logMessage("Calculator::calcPosition");
    double x = 0, y = 0;
    int size = b_matrix.size();
    if(cat_matrix.size() != 2*size){
        Logger::logMessage("Calculator::calcPosition::ERROR!!!");
        return false;
    }
    for(int i = 0; i < size; ++i){
        x += cat_matrix[i]*b_matrix[i];
        y += cat_matrix[size+i]*b_matrix[i];
    }
    position = {x,y};
    return true;
}

void Calculator::applyFilter(Point& point){
    Logger::logMessage("Calculator::applyFilter");
    if(m_last_points.length() < 3){
        m_last_points.append(point);
    }
    if(m_last_points.length() > 2){
        point.x = 0;
        point.y = 0;
        for(Point& p: m_last_points){
            point.x += p.x;
            point.y += p.y;
        }
        point.x /= 3;
        point.y /= 3;
        m_last_points.erase(m_last_points.begin());
    }
}

void Calculator::calcMedian(const QString& mac_address, qint16& rssi){
    Logger::logMessage("Calculator::calcMedian");
    if(m_median_cache[mac_address].length() < 3){
        m_median_cache[mac_address].append(rssi);
    }
    if(m_median_cache[mac_address].length() > 2){
        QList<qint16> rssi_cache = m_median_cache[mac_address];
        qSort(rssi_cache);
        rssi = rssi_cache[1];
        Logger::logMessage("median_rssi = " + QString::number(rssi));
        m_median_cache[mac_address].erase(m_median_cache[mac_address].begin());
    }
}
