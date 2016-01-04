<?php

/**
 * @file
 * Topic menu.
 */


$plugin = array(
  'single' => TRUE,
  'title' => t('Topic header'),
  'description' => t('Header for the topic.'),
  'category' => t('Swift Framework'),
);


/**
 * Render the custom content type.
 */
function swift_framework_topic_header_content_type_render($subtype, $conf, $panel_args, $context) {

  $contentNavigation = ContentNavigation::getInstance();
  $currentPage = $contentNavigation->getCurrentPage();

  if ($currentPage->hasTopicContext()) {

    $node = $currentPage->getCurrentTopic();
    if (!$node) {
      return;
    }
    $node_viewed = node_view($node, 'header');

    $block = new stdClass();
    $block->module  = 'swift_framework';
    //$block->delta   = 'form';
    $block->title   = '';
    $block->content = $node_viewed;

    return $block;

  }

  return;

}

/**
 * Returns an edit form for custom type settings.
 */
function swift_framework_topic_header_content_type_edit_form($form, &$form_state) {
  // Empty so that we can have title override.
  return $form;
}

/**
 * Submit handler for master_topics form.
 */
function swift_framework_topic_header_content_type_edit_form_submit($form, &$form_state) {
  foreach (array_keys($form_state['plugin']['defaults']) as $key) {
    $form_state['conf'][$key] = $form_state['values'][$key];
  }
}

/**
 * Returns the administrative title for a type.
 */
function swift_framework_topic_header_content_type_admin_title($subtype, $conf, $context) {
  return t('Topic header');
}