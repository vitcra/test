<?php

/**
 * Implements hook_ds_fields_info().
 */
function swift_framework_ds_fields_info($entity_type) {

  $fields = array();
  $file = drupal_get_path('module', 'swift_framework') . '/includes/ds.inc';
  $topic_types = variable_get('swift_topic_types', array());
  $child_types = variable_get('swift_topic_child_types', array());


  $ui_limits = array();
  foreach ($topic_types as $type) {
    $ui_limits[] = $type . '|*';
  }

  // Basically this is the topic language switcher
  $fields['node']['available_translations'] = array(
    'title' => t('Also available in other languages'),
    'field_type' => DS_FIELD_TYPE_FUNCTION,
    'function' => '_swift_framework_ds_available_translations',
    'file' => $file,
    'ui_limit' => $ui_limits,
  );

  // Warning message for missing translation
  $ui_limits = array();
  foreach ($child_types as $type) {
    $ui_limits[] = $type . '|*';
  }
  $fields['node']['no_translation_warning'] = array(
    'title' => t('No translation warning'),
    'field_type' => DS_FIELD_TYPE_FUNCTION,
    'function' => '_swift_framework_ds_no_translation_warning',
    'file' => $file,
    'ui_limit' => $ui_limits,
  );

  // Right hand topic column
  $ui_limits = array('page|*');
  $fields['node']['topic_right_col'] = array(
    'title' => t('Topic right hand column'),
    'field_type' => DS_FIELD_TYPE_FUNCTION,
    'function' => '_swift_framework_ds_topic_right_col',
    'file' => $file,
    'ui_limit' => $ui_limits,
  );

  $ui_limits = array();
  foreach ($child_types as $type) {
    $ui_limits[] = $type . '|*';
  }
  // Topic footer
  $fields['node']['topic_footer'] = array(
    'title' => t('Topic footer'),
    'field_type' => DS_FIELD_TYPE_FUNCTION,
    'function' => '_swift_framework_ds_topic_footer',
    'file' => $file,
    'ui_limit' => $ui_limits,
  );
  //field for finding available translated resources
  $fields['node']['resource_translated'] = array(
    'title' => t('Resource translated'),
    'field_type' => DS_FIELD_TYPE_FUNCTION,
    'function' => '_swift_framework_ds_resource_translated',
    'file' => $file,
    'ui_limit' => array(
      'resource|*',
    ),
  );
  return $fields;
}