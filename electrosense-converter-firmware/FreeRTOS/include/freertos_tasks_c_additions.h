UBaseType_t uxYieldPending( void )
{
    UBaseType_t isIt = xYieldPending;
    xYieldPending = pdFALSE;

    return isIt;
}

xTaskHandle xGetCurrentTaskHandle( void )
{
    xTaskHandle xReturn;

    portENTER_CRITICAL();
    xReturn = ( xTaskHandle ) pxCurrentTCB;
    portEXIT_CRITICAL();

    return xReturn;
}
