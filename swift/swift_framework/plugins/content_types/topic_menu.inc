<?php

/**
 * @file
 * Topic menu.
 */


$plugin = array(
  'single' => TRUE,
  'title' => t('Topics menu'),
  'description' => t('Menu for a topic.'),
  'category' => t('Swift Framework'),
);

/**
 * Wrapper around l function
 * to make it a bit smarter
 *
 * it is difficult to get a link with l
 * so that it is aliased in a desired language
 * and still active
 */
function _swift_framework_l($title, $path, $language, $options = array()) {
  // Append active class.
  if ($path == $_GET['q']) {
    $options['attributes']['class'][] = 'active';
  }
  $path = drupal_get_path_alias($path, $language);
  return l($title, $path, $options);
}

/**
 * Render the custom content type.
 */
function swift_framework_topic_menu_content_type_render($subtype, $conf, $panel_args, $context) {

  $contentNavigation = ContentNavigation::getInstance();
  $currentPage = $contentNavigation->getCurrentPage();

  if ($currentPage->hasTopicContext()) {

    $default_language = language_default('language');

    $topic = $currentPage->getCurrentTopic();

    // generate the Overview tab that links to topic
    $overview_tab_title = _swift_framework_get_tab_title($topic, 'field_overview_tab_title');
    if (empty($overview_tab_title)) {
      $overview_tab_title = t('Overview');
    }

    $links = array(_swift_framework_l($overview_tab_title, 'node/'. $topic->nid, $topic->language, array('fragment' => 'topic-tabs-menu')));
    $c = 0;

    // prepare for active trail calculation
    $topic_menu = $currentPage->getCurrentTopicMenu();
    $plid = empty($topic_menu->child_link['plid']) ? 0 : $topic_menu->child_link['plid'];

    foreach ($currentPage->getTopicLinks() as $link) {
      $options = array('fragment' => 'topic-tabs-menu');
      if (!empty($link->query)) {
        $options['query'] = $link->query;
        $alias_language = $default_language;
      }
      else {
        $alias_language = $topic->language;
      }
      $li = array('data' => _swift_framework_l($link->link_title, $link->link_path, $alias_language, $options));
      if ($plid && $link->mlid == $plid) {
        $li['class'] = array('active-trail');
      }

      $links[] = $li;

      $c++;
      if ($c > 6) {
        break;
      }
    }

    $block = new stdClass();
    $block->module  = 'swift_framework';
    //$block->delta   = 'form';
    $block->title   = '';
    $block->content = array(
      '#theme' => 'item_list',
      '#items' => $links,
      '#attributes' => array('class' => array('menu nav navbar-nav'), 'id' => 'topic-tabs-menu'),
    );

    return $block;

  }

  return;

}

/**
 * Returns an edit form for custom type settings.
 */
function swift_framework_topic_menu_content_type_edit_form($form, &$form_state) {
  // Empty so that we can have title override.
  return $form;
}

/**
 * Submit handler for topic_menu form.
 */
function swift_framework_topic_menu_content_type_edit_form_submit($form, &$form_state) {
  foreach (array_keys($form_state['plugin']['defaults']) as $key) {
    $form_state['conf'][$key] = $form_state['values'][$key];
  }
}

/**
 * Returns the administrative title for a type.
 */
function swift_framework_topic_menu_content_type_admin_title($subtype, $conf, $context) {
  return t('Topic menu');
}