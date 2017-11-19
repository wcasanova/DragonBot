////////////////////////////////////////////////////////////////
//Este fichero contendrá todas las funciones de velocidades y //
//aceleraciones que tendrá el robot.                          //
////////////////////////////////////////////////////////////////


/**
* Establece la velocidad de los motores, luego de comprobar si es necesario realizar
* una aceleración.
* @param correccion Variación de la velocidad calculada por el PID
*/
void set_speed(double correccion) {

  calc_accel(velBase, false, 500, false);

  int velI = velReal + correccion;
  int velD = velReal - correccion;

  if(stop_emergencia){
    velI = 0;
    velD = 0;
    velVentilador = 0;
    kp = 0;
    ki = 0;
    kd = 0;
  }

  // Limitar velocidad del motor derecho
  if (velD > 255) {
    velD = 255;
  } else if (velD < 0) {
    velD = 0;
  }

  // Limitar velocidad del motor izquierdo
  if (velI > 255) {
    velI = 255;
  } else if (velI < 0) {
    velI = 0;
  }

  digitalWrite(MOTOR_DERECHO_ADELANTE, LOW);
  digitalWrite(MOTOR_DERECHO_ATRAS, LOW);
  digitalWrite(MOTOR_IZQUIERDO_ADELANTE, LOW);
  digitalWrite(MOTOR_IZQUIERDO_ATRAS, LOW);

  digitalWrite(MOTOR_DERECHO_ADELANTE, HIGH);
  digitalWrite(MOTOR_IZQUIERDO_ADELANTE, HIGH);

  analogWrite(MOTOR_DERECHO_PWM   , velD);
  analogWrite(MOTOR_IZQUIERDO_PWM , velI);
}

/**
* Función de calculo de la aceleración, en caso de que la velocidad deseada no sea igual a la velocidad actual del robot
* @param vel              Velocidad deseada para el robot
* @param forzar_calculo   Indica si se realiza de nuevo el cálculo de la pendiente de la recta de aceleración
* @param time             Tiempo que tardará en realizar la aceleración
* @param forzar_velocidad Indica si se salta la aceleración y asigna la velocidad de inmediato.
*/
void calc_accel(short vel, bool forzar_calculo, short time, bool forzar_velocidad) {
  if ((abs(vel - velReal) > 40 || cambiando_vel) && vel != 0 && !forzar_velocidad) {

    // Calcula la pendiente en caso de primera ejecucion de movimiento (si la pendiente de ambos motores es 0) o si se especifica forzado de calculo
    // TODO: Comprobar comportamiento cuando no es necesaria actualizacion => Cuando la vel objetivo es igual a la vel actual

    if (((m == 0) && vel != vel_ini) || forzar_calculo) {
      // Si el robot ya estaba en movimiento obtiene sus velocidades reales, de las que parte para aceleración. Si no, usa el PWM_INICIAL.
      vel_ini = velReal;

      // Calcula la pendiente si la vel objetivo es distinta a la vel inicial (deberia solventar lo anterior de aceleraciones con vels iguales)

      m = (vel - (float)vel_ini) / (float)time;

      // Asigna a la vel real el valor inicial de aceleracion (para cuando empieza parado, darle velocidad inicial)
      // e indica que se esta cambiando_vel (acelerando o frenando) y guarda los millis del inicio de aceleracion
      cambiando_vel = true;
      millis_inicial_accel = millis();

    }

    // Asigna la velocidad en aceleracion correspondiente al tiempo transcurrido
    if (cambiando_vel) {
      // y   = 	mx    +    b
      // vel = m*millis + vel ini
      velReal = ((millis() - millis_inicial_accel) * m) + (float)vel_ini;

    }

    // Establece la velocidad en el máximo (objetivo) cuando pasa el tiempo de aceleracion, y deshabilita la asignación de velocidad
    if ((millis() - millis_inicial_accel) >= time) {
      cambiando_vel = false;
      velReal = vel;
      m = 0;
    }
  } else {
    velReal = vel;
    m = 0;
    cambiando_vel = false;
  }
}

/**
* Función de calculo del ideal para ajustarlo progresivamente y evitar que el robot dé sacudidas
* @param ideal_obj        Ideal deseado para el robot
* @param forzar_calculo   Indica si se realiza de nuevo el cálculo de la pendiente de la recta de aceleración
* @param time             Tiempo que tardará en realizar el cambio de ideal
* @param forzar_ideal     Indica si se salta el cambio y asigna el ideal de inmediato.
*/
void calc_ideal(short ideal_obj, bool forzar_calculo, short time, bool forzar_ideal) {
  if ((abs(ideal_obj - ideal) > 40 || cambiando_ideal) && !forzar_ideal) {

    // Calcula la pendiente si el ideal objetivo es muy distinto al actual o si se especifica forzado de calculo
    // TODO: Comprobar comportamiento cuando no es necesaria actualizacion => Cuando la ideal objetivo es igual a la ideal actual

    if (((m_ideal == 0) && ideal_obj != ideal_ini) || forzar_calculo) {
      // Obtiene su ideal actual, del que partirá para la realización de cambios.
      ideal_ini = ideal;

      // Calcula la pendiente si el ideal objetivo es distinto al ideal inicial
      m_ideal = (ideal_obj - (float)ideal_ini) / (float)time;

      // Indica que se esta cambiando_ideal y guarda los millis del inicio del cambio
      cambiando_ideal = true;
      millis_inicial_ideal = millis();

    }

    // Asigna el ideal correspondiente al tiempo transcurrido
    if (cambiando_ideal) {
      // y   = 	mx    +    b
      // ideal = m*millis + ideal_ini
      ideal = ((millis() - millis_inicial_ideal) * m_ideal) + (float)ideal_ini;

    }

    // Establece el ideal en el máximo (objetivo) cuando pasa el tiempo de cambio, e indica el fin del cambio de ideal.
    if ((millis() - millis_inicial_ideal) >= time) {
      cambiando_ideal = false;
      ideal = ideal_obj;
      m_ideal = 0;
    }
  } else {
    ideal = ideal_obj;
    ideal_ini = ideal;
    m_ideal = 0;
    cambiando_ideal = false;
    digitalWrite(LED, HIGH);
  }
}

/**
* Realiza el cálculo de la corrección de los motores mediante el PID
* @param  position Posición del cálculo anterior
* @return          Posición actual del robot
*/
double calc_PID(double position) {
  double p = 0;
  double i = 0;
  double d = 0;
  filtro_ideal.Filter(ideal);
  double error = filtro_ideal.Current() - position;

  // Proporcional: constante · desviación
  p = kp/100.0 * error;

  // Derivada: constante · derivada del error
  if(last_pid_calc>0)
  // d = kd/100.0 * ((error - last_error) / (micros() - last_pid_calc));
  d = kd/100.0 * (error - last_error);

  if((error < 100 && ki!=0) && (abs(sum_error) < 3000 || ((sum_error > 0 && error < 0) || (sum_error < 0 && error > 0)))){
    sum_error += error;
  }
  if(error > 100){
    sum_error = 0;
  }

  // i = ki/100.0 * sum_error;

  last_pid_calc = micros();
  last_error = error;

  return p + i + d;
}

long last_cambio_carril = 0;
void check_laser_sensors(){
  read_laser_sensor(LASER_FRONTAL);
  if(robots_sensor_values[LASER_FRONTAL] < 500 && (millis()-last_cambio_carril) > 2000){
    id_ob = -id_ob;
    last_cambio_carril = millis();
    digitalWrite(LED, LOW);
  }
}
