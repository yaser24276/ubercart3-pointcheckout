<?php

/**
 * @file
 * PointCheckout administration menu items.
 */

/**
 * Processes Instant Payment Notifiations from PointCheckout.
 */
function uc_pointcheckout_success() {
  if (!isset($_GET['reference'])) {
    watchdog('uc_pointcheckout', 'verify attempted with invalid order id.', array(), WATCHDOG_ERROR);
    return;
  }
  if (!isset($_GET['checkout'])) {
      watchdog('uc_pointcheckout', 'verify attempted with invalid checkout id.', array(), WATCHDOG_ERROR);
      return;
  }
  
    $order_id = intval($_GET['reference']);


  watchdog('uc_pointcheckout', 'Receiving verify at URL for order @order_id. <pre>@debug</pre>', array('@order_id' => $order_id));

  $order = uc_order_load($order_id);

   if ($order == FALSE) {
        watchdog('uc_pointcheckout', 'verify attempted for non-existent order @order_id.', array('@order_id' => $order_id), WATCHDOG_ERROR);
        return;
   }

   $checkoutId = check_plain($_GET['checkout']);
   $_BASE_URL= getPointCheckoutApiUrl();
   $headers = array(
       'Content-Type: application/json',
       'Api-Key:'.variable_get('uc_pointcheckout_api_key', ''),
       'Api-Secret:'.variable_get('uc_pointcheckout_api_secret', ''),
   );
   $ch = curl_init($_BASE_URL.'/'.$_GET['checkout']);
   curl_setopt($ch, CURLOPT_RETURNTRANSFER, 1);
   curl_setopt($ch, CURLOPT_HTTPHEADER, $headers);
   
   $response = curl_exec($ch);
   
  // Log any errors to the watchdog.
  if ($error = curl_error($ch)) {
    watchdog('uc_pointcheckout', 'payment verify failed with cURL error: @error', array('@error' => $error), WATCHDOG_ERROR);
    return;
  }

  curl_close($ch);
  $message = '';
  $response_info = json_decode($response);
  $payment_amount=$response_info->result->grandtotal - $response_info->result->cod ;
  $payment_currency =$response_info->result->currency;
  
  
  //check response and redirect user to either success or failure page
  if (($response_info->success == 'true' && $response_info->result->status =='PAID')) {
      $message.= getPointCheckoutOrderHistoryMessage($response_info->result->checkoutId,$response_info->result->cod,$response_info->result->status,$payment_currency);
      uc_payment_enter($order_id, 'pointcheckout_wps', $payment_amount, $order->uid, null, $message);
      uc_cart_complete_sale($order);
      uc_order_comment_save($order_id, 0, t($message));
      uc_order_update_status($order_id,'payment_received');
      drupal_set_message(t('Thank you for using pointcheckout your payment has been successfully processed'));
      db_insert('uc_payment_pointcheckout_pay')
      ->fields(array(
          'order_id' => $order_id,
          'checkout_id' => $response_info->result->checkoutId,
          'amount' => $response_info->result->grandtotal,
          'status' => $response_info->result->status,
          'received' => REQUEST_TIME,
      ))
      ->execute();
      $_SESSION['uc_checkout'][$order_id]['do_complete'] = TRUE;
      drupal_goto('cart/checkout/complete');
  }elseif(!$response_info->success == 'true'){
      uc_order_comment_save($order_id, 0, t('PointCheckout Payment Failed  error: '.$response_info->error), 'admin');
      watchdog('uc_pointcheckout','[ERROR} PointCheckout response with error payment failed   error msg is :'.$response_info->error);
      uc_order_update_status($order_id,'canceled');
      drupal_set_message(t('payment failed order canceled.'),'error');
      drupal_goto('cart');
  }else{
      uc_order_comment_save($order_id, 0, t('PointCheckout Payment did not complete,payment is : CANCELED'), 'admin');
      watchdog('uc_pointcheckout','[ERROR} PointCheckout response that payment status you try to verify did not completed successfully' );
      uc_order_update_status($order_id,'canceled');
      drupal_set_message(t('payment failed order canceled.'),'error');
      drupal_goto('cart');
  }
  
}
 
  
/**
 * Processes Instant Payment Notifiations from PointCheckout.
 */
function uc_pointcheckout_cancel() {
    if (!isset($_GET['reference'])) {
        watchdog('uc_pointcheckout', 'cancel attempted with invalid order id.', array(), WATCHDOG_ERROR);
        return;
    }
    $order_id = intval($_GET['reference']);
    watchdog('uc_pointcheckout', 'Receiving verify at URL for order @order_id. <pre>@debug</pre>', array('@order_id' => $order_id));
    $order = uc_order_load($order_id);
    if ($order == FALSE) {
        watchdog('uc_pointcheckout', 'cancel attempted for non-existent order @order_id.', array('@order_id' => $order_id), WATCHDOG_ERROR);
        return;
    }
        uc_order_comment_save($order_id, 0, t('user had cancelled the payment'), 'admin');
        uc_order_update_status($order_id,'canceled');        
        watchdog('uc_pointcheckout','[ERROR} PointCheckout response with error payment failed');
        //unset($_SESSION['cart_order']);
        drupal_set_message(t('order canceled.'),'warning');
        drupal_goto('cart');
}






