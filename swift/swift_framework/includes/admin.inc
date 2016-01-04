<?php

/**
 * @file
 * Administration logic for the Swift Framework.
 */

function swift_framework_content_settings_form() {

  $form = array();

  $options = node_type_get_names();

  $form['swift_master_types'] = array(
    '#type' => 'checkboxes',
    '#title' => t('Swift master types'),
    '#options' => $options,
    '#default_value' => variable_get('swift_master_types', array()),
  );

  $form['swift_topic_types'] = array(
    '#type' => 'checkboxes',
    '#title' => t('Swift topic types'),
    '#options' => $options,
    '#default_value' => variable_get('swift_topic_types', array()),
  );

  $form['swift_topic_child_types'] = array(
    '#type' => 'checkboxes',
    '#title' => t('Swift topic child types'),
    '#options' => $options,
    '#default_value' => variable_get('swift_topic_child_types', array()),
  );

  // this will filter out empty values
  // see variable_settings_form_submit()
  $form['array_filter'] = array(
    '#type' => 'hidden',
    '#value' => 1,
  );

  return system_settings_form($form);

}

