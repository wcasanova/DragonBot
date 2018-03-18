/**
 * Función para configurar el Timer 2 para cálculo de posición y PID
 */
void inicia_timer_PID(){
    TimerPID.pause();
    TimerPID.setPeriod(250);
    TimerPID.setMode(TIMER_CH2, TIMER_OUTPUT_COMPARE);
    TimerPID.setCompare(TIMER_CH2, 1);
    TimerPID.attachInterrupt(1, handler_timer_PID);
    TimerPID.refresh();
    TimerPID.resume();
}

/**
 * Función a la que llama el Timer 2 para realizar el cálculo de posición y PID
 */
void handler_timer_PID() {
  posicionActual = calcular_posicion(posicionActual);
  correccion = calcular_PID(posicionActual);
  if(!enCompeticion || competicionIniciada){
    dar_velocidad(correccion);
  }
}

/**
 * Función para configurar el Timer 3 para la calibración Serial-BT
 */
void inicia_timer_BT(){
    TimerBT.pause();
    TimerBT.setPeriod(50000);
    TimerBT.setMode(TIMER_CH1, TIMER_OUTPUT_COMPARE);
    TimerBT.setCompare(TIMER_CH1, 1);
    TimerBT.attachInterrupt(1, handler_timer_BT);
    TimerBT.refresh();
    TimerBT.resume();
}

/**
 * Función a la que llama el Timer 3 ajustar la configuración via Serial-BT
 */
void handler_timer_BT() {
  CalibracionPID.update();
  nivel_bateria(true);
}
