<?php

/**
 * @file
 * Plugin to provide access control based upon node type.
 */

/**
 * Plugins are described by creating a $plugin array which will be used
 * by the system that includes this file.
 */
$plugin = array(
  'title' => t('Topic: type'),
  'description' => t('Control access by topic type.'),
  'callback' => 'swift_framework_topic_type_ctools_access_check',
  'default' => array('type' => array()),
  'settings form' => 'swift_framework_topic_type_ctools_access_settings',
  'settings form submit' => 'swift_framework_topic_type_ctools_access_settings_submit',
  'summary' => 'swift_framework_topic_type_ctools_access_summary',
  'required context' => new ctools_context_required(t('Topic'), 'topic_context'),
  'restrictions' => 'topic_context_type_ctools_access_restrictions',
);

/**
 * Settings form for the 'by topic_type' access plugin
 */
function swift_framework_topic_type_ctools_access_settings($form, &$form_state, $conf) {
  $types = node_type_get_types();
  $topic_types = variable_get('swift_topic_types', array());

  foreach ($topic_types as $type) {
    $options[$type] = check_plain($types[$type]->name);
  }

  $form['settings']['type'] = array(
    '#title' => t('Topic type'),
    '#type' => 'checkboxes',
    '#options' => $options,
    '#description' => t('Only the checked topic types will be valid.'),
    '#default_value' => $conf['type'],
  );
  return $form;
}

/**
 * Compress the node_types allowed to the minimum.
 */
function swift_framework_topic_type_ctools_access_settings_submit($form, &$form_state) {
  $form_state['values']['settings']['type'] = array_filter($form_state['values']['settings']['type']);
}

/**
 * Check for access.
 */
function swift_framework_topic_type_ctools_access_check($conf, $context) {
  // As far as I know there should always be a context at this point, but this
  // is safe.
  if (empty($context) || empty($context->data) || empty($context->data->type)) {
    return FALSE;
  }

  if (array_filter($conf['type']) && empty($conf['type'][$context->data->type])) {
    return FALSE;
  }

  return TRUE;
}

/**
 * Inform the UI that we've eliminated a bunch of possibilities for this
 * context.
 */
function swift_framework_topic_type_ctools_access_restrictions($conf, &$context) {
  if (isset($context->restrictions['type'])) {
    $context->restrictions['type'] = array_unique(array_merge($context->restrictions['type'], array_keys(array_filter($conf['type']))));
  }
  else {
    $context->restrictions['type'] = array_keys(array_filter($conf['type']));
  }
}

/**
 * Provide a summary description based upon the checked node_types.
 */
function swift_framework_topic_type_ctools_access_summary($conf, $context) {
  if (!isset($conf['type'])) {
    $conf['type'] = array();
  }
  $types = node_type_get_types();

  $names = array();
  // If a node type doesn't exist, let the user know, but prevent a notice.
  $missing_types = array();

  foreach (array_filter($conf['type']) as $type) {
    if (!empty($types[$type])) {
      $names[] = check_plain($types[$type]->name);
    }
    else {
      $missing_types[] = check_plain($type);
    }
  }

  if (empty($names) && empty($missing_types)) {
    return t('@identifier is any node type', array('@identifier' => $context->identifier));
  }

  if (!empty($missing_types)) {
    $output = array();
    if (!empty($names)) {
      $output[] = format_plural(count($names), '@identifier is type "@types"', '@identifier type is one of "@types"', array('@types' => implode(', ', $names), '@identifier' => $context->identifier));
    }
    $output[] = format_plural(count($missing_types), 'Missing/ deleted type "@types"', 'Missing/ deleted type is one of "@types"', array('@types' => implode(', ', $missing_types)));
    return implode(' | ', $output);
  }

  return format_plural(count($names), '@identifier is type "@types"', '@identifier type is one of "@types"', array('@types' => implode(', ', $names), '@identifier' => $context->identifier));
}

