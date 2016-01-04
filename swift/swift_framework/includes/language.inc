<?php
/**
 * @file
 * Contains logic related to the custom language manipulations we handle.
 */

/**
 * Implements hook_language_types_info_alter().
 *
 * Language_url type is a fixed type, hooking in is a bit more tedious
 */
function swift_framework_language_types_info_alter(&$language_types) {
  array_unshift($language_types['language_url']['fixed'], 'swift_framework_language_provider');
}

/**
 * Implements hook_language_negotiation_info().
 */
function swift_framework_language_negotiation_info() {
  return array(
    'swift_framework_language_provider' => array(
      'types' => array('language_url', 'language', 'language_content'),
      'callbacks' => array(
        'language' => 'swift_framework_language_provider',
      ),
      'weight' => 0,
      'file' => drupal_get_path('module', 'swift_framework') . '/includes/language.inc',
      'name' => t('Language from node'),
      'description' => t('Current node language (if any) or the language given by the prefix in the alias of a translated node.'),
      'cache' => 0,
    ),
  );
}

/**
 * Callback for the swift_framework_language_provider
 * @see swift_framework_language_negotiation_info().
 *
 * Get language from node if path like node/xxx
 * otherwise from language prefix
 *
 * Important we rely on the fact that language prefix is part of actual alias
 * instead of a result of an url rewrite or something
 *
 */
function swift_framework_language_provider($languages) {
  $langcode = arg(0);
  if (isset($languages[$langcode])) {
    return $langcode;
  }

  if (arg(0) === 'node') {
    $nid = (int) arg(1);
    return db_query('SELECT language FROM {node} WHERE nid = :nid', array(':nid' => $nid))->fetchField();
  }

  return FALSE;
}