<?php

/**
 * Removes the last element of the active trail if it matches the current item.
 * `menu_get_active_breadcrumb()` already does this for us but it seems that in some
 * cases it does it a bit too late.
 *
 * @param $active_trail
 * @param $item
 */
function _swift_framework_breadcrumbs_remove_last(&$active_trail, $item) {
  if ($item['path'] !== 'node/%') {
    return;
  }

  $last = end($active_trail);
  $remove_last = false;

  // If it's the same exact node
  if ($item['href'] === $last['href']) {
    $remove_last = true;
  }

  // If it's a translation
  if (_swift_framework_breadcrumbs_is_item_translation($last, $item)) {
    $remove_last = true;
  }

  if ($remove_last) {
    array_pop($active_trail);
  }
}

/**
 * When we are on a topic child page that either has a `tl` in the URL (non translated child)
 * or the on a topic child that is translated, change the breadcrumb to the
 * parent (topic) to reflect this language.
 *
 * @param $active_trail
 * @param $item
 */
function _swift_framework_breadcrumbs_child_parent_language(&$active_trail, $item) {
  $contentNavigation = ContentNavigation::getInstance();
  $currentPage = $contentNavigation->getCurrentPage();

  if (!$currentPage || !$currentPage->hasTopicContext() || !$currentPage->isTopicChildPage()) {
    return;
  }

  $needs_language_adjustment = false;
  if (isset($_GET['tl'])) {
    $needs_language_adjustment = true;
  }
  $child = $currentPage->getContent();
  if ($child->tnid !== '0' && $child->tnid !== $child->nid) {
    $needs_language_adjustment = true;
  }


  if (!$needs_language_adjustment) {
    return;
  }

  // Requirement is that breadcrumb is made of menu titles
  $old = array_pop($active_trail);
  $topic = $currentPage->getCurrentTopic();
  $active_trail[] = array(
    'title' => $old['title'], //$topic->title,
    'href' => 'node/' . $topic->nid,
    'localized_options' => array(),
  );
}

/**
 * This handles the cases of Topic grand-child pages by adding to
 * the active trail the link to the topic child page that is the parent of the grand-child
 *
 * No longer needed see SDC-404
 *
 * @param $active_trail
 */
/*function _swift_framework_breadcrumbs_for_grandchildren(&$active_trail, $item) {
  if ($item['path'] !== 'node/%') {
    return;
  }

  $contentNavigation = ContentNavigation::getInstance();
  $currentPage = $contentNavigation->getCurrentPage();

  if (!$currentPage || !$currentPage->hasTopicContext() || !$currentPage->isTopicChildPage()) {
    return;
  }

  $topic_menu = $currentPage->getCurrentTopicMenu();
  if (!$topic_menu || !isset($topic_menu->child_link)) {
    return;
  }

  $plid = $topic_menu->child_link['plid'];
  if ($plid === "0") {
    return;
  }

  $lang = false;
  $needs_language_adjustment = false;
  if (isset($_GET['tl'])) {
    $needs_language_adjustment = true;
    $lang = $_GET['tl'];
  }
  $grandchild = $currentPage->getContent();
  if ($grandchild->tnid !== '0' && $grandchild->tnid !== $grandchild->nid) {
    $needs_language_adjustment = true;
    $lang = $grandchild->language;
  }

  $topic_links = $currentPage->getTopicLinks();
  $parent_menu = $topic_links[$plid];

  // If the breadcrumb to the parent (child) needs no lang adjustment
  if (!$needs_language_adjustment) {
    $trail = array(
      'title' => $parent_menu->link_title,
      'href' => $parent_menu->link_path,
      'localized_options' => array(),
    );

    $active_trail[] = $trail;
    return;
  }

  // If there is need for lang adjustment and the parent node is translated
  $translations = translation_node_get_translations($parent_menu->nid);
  if ($translations && $lang && isset($translations[$lang])) {
    $parent = $translations[$lang];
    $trail = array(
      'title' => $parent->title,
      'href' => 'node/' . $parent->nid,
      'localized_options' => array(),
    );

    $active_trail[] = $trail;
    return;
  }

  // If there is need for lang adjustment and the parent node is not translated
  // we add the `tl` to the path
  if ((!$translations && $lang) || (count($translations) === 1 && isset($translations['en']) && $lang)) {
    if (isset($translations['en'])) {
      $parent = $translations['en'];
      $trail = array(
        'title' => $parent->title,
        'href' => 'node/' . $parent->nid,
        'localized_options' => array('query' => array('tl' => $lang)),
      );

      $active_trail[] = $trail;
      return;
    }
    else {
      $trail = array(
        'title' => $parent_menu->link_title,
        'href' => $parent_menu->link_path,
        'localized_options' => array('query' => array('tl' => $lang)),
      );

      $active_trail[] = $trail;
      return;
    }
  }
}*/

/**
 * Handles the cases when topics are not in a menu so their children have no
 * default way of showing a breadcrumb upwards. Here we just add the topic title
 * to the breadcrumb. The topics themselves will only have `Home` (which comes by
 * default)
 */
function _swift_framework_breadcrumbs_for_menuless_topic_children(&$active_trail, $item) {
  if ($item['path'] !== 'node/%') {
    return;
  }

  $contentNavigation = ContentNavigation::getInstance();
  $currentPage = $contentNavigation->getCurrentPage();

  if (!$currentPage || !$currentPage->hasTopicContext() || !$currentPage->isTopicChildPage()) {
    return;
  }

  $menu_name = $currentPage->getTopicMenuName();
  if ($menu_name) { // If we get a menu name we don't need to do anything.
    return;
  }

  $topic = $currentPage->getCurrentTopic();

  if ($topic->type == 'topic_event') {
    $active_trail[] = array(
      'title' => t('Insights'),
      'href' => 'insights',
      'localized_options' => array(),
    );

    $active_trail[] = array(
      'title' => t('Events'),
      'href' => 'insights/events',
      'localized_options' => array(),
    );
  }

  $trail = array(
    'title' => $topic->title,
    'href' => 'node/' . $topic->nid,
    'localized_options' => array(),
  );

  // This might be deprecated code. Uncommenting for now as it messes with
  // breadcrumb functionality for event child pages.
//  if (count($active_trail) > 1) {
//    $last = end($active_trail);
//    array_pop($active_trail);
//    $active_trail[] = $trail;
//    $active_trail[] = $last;
//
//    return;
//  }

  $active_trail[] = $trail;
}

/**
 * Determines whether the current item in the active trail is a path to a node
 * that is a translation of the last item in the trail.
 *
 * @see _swift_framework_breadcrumbs_remove_last().
 * @param $last
 * @param $item
 *
 * @return bool
 */
function _swift_framework_breadcrumbs_is_item_translation($last, $item) {
  if ($item['path'] !== 'node/%') {
    return false;
  }

  $node = $item['page_arguments'][0];
  if ('node/' . $node->tnid === $last['href']) {
    return true;
  }

  return false;
}

/**
 * Handles the breadcrumb alterations for News nodes.
 *
 * @param $active_trail
 * @param $item
 */
function _swift_framework_breadcrumbs_for_news(&$active_trail, $item) {
  if ($item['path'] !== 'node/%') {
    return;
  }

  $node = $item['page_arguments'][0];
  if ($node->type !== 'news') {
    return;
  }

  if (!$node->field_channel_category) {
    return;
  }

  // Add the News item with a link to the insights page.
  $active_trail[] = array(
    'title' => t('Insights'),
    'href' => 'insights',
    'localized_options' => array(),
  );

  $category = $node->field_channel_category[LANGUAGE_NONE][0]['tid'];
  $channel = db_query("SELECT td.name, td.tid FROM {taxonomy_term_data} td JOIN {taxonomy_term_hierarchy} th ON th.parent = td.tid WHERE th.tid = :tid", array(':tid' => $category))->fetch();
  if (!$channel) {
    return;
  }

  // Add the channel.
  $active_trail[] = array(
    'title' => check_plain($channel->name),
    'href' => 'taxonomy/term/' . $channel->tid,
    'localized_options' => array(),
  );
}


/**
 * Handles the breadcrumb alterations for News nodes.
 *
 * @param $active_trail
 * @param $item
 */
function _swift_framework_breadcrumbs_for_topics(&$active_trail, $item) {
  if ($item['path'] !== 'node/%') {
    return;
  }

  // Only influence topic types:
  $node = $item['page_arguments'][0];
  if (!swift_framework_type_is('topic', $node->type)) {
    return;
  }

  // Get the defined menu title:
  $menu_title = menu_get_active_title();

  // No menu title, do nothing:
  if (empty($menu_title)) {
    return;
  }

  // Add the topic title without a link
  // This works because of code in _swift_framework_breadcrumbs().
  // Technique and code borrowed from special_menu_items contrib module.
  $active_trail[] = array(
    'title' => $menu_title,
    'href' => '<nolink>', // This is a special token that we use to recognize this special item
    'localized_options' => array(),
  );
}

/**
 * Handles the breadcrumb alterations for Webinar nodes.
 *
 * @param $active_trail
 * @param $item
 */
function _swift_framework_breadcrumbs_for_webinars(&$active_trail, $item) {
  if ($item['path'] !== 'node/%') {
    return;
  }

  $node = $item['page_arguments'][0];
  if ($node->type !== 'webinar') {
    return;
  }

  $active_trail[] = array(
    'title' => t('Insights'),
    'href' => 'insights',
    'localized_options' => array(),
  );

  $active_trail[] = array(
    'title' => t('Webinars'),
    'href' => 'insights/webinars',
    'localized_options' => array(),
  );

}

/**
 * Handles the breadcrumb alterations for Topic Event nodes.
 *
 * @param $active_trail
 * @param $item
 */
function _swift_framework_breadcrumbs_for_events(&$active_trail, $item) {
  if ($item['path'] !== 'node/%') {
    return;
  }

  $node = $item['page_arguments'][0];
  if ($node->type !== 'topic_event') {
    return;
  }

  $active_trail[] = array(
    'title' => t('Insights'),
    'href' => 'insights',
    'localized_options' => array(),
  );

  $active_trail[] = array(
    'title' => t('Events'),
    'href' => 'insights/events',
    'localized_options' => array(),
  );

}

/**
 * Handles the breadcrumb alterations for Free Text nodes.
 */
function _swift_framework_breadcrumbs_for_freetext(&$active_trail, $item) {
  if ($item['path'] !== 'node/%') {
    return;
  }

  $node = $item['page_arguments'][0];
  if ($node->type !== 'page') {
    return;
  }

  $contentNavigation = ContentNavigation::getInstance();
  $currentPage = $contentNavigation->getCurrentPage();

  if (!$currentPage || $currentPage->hasTopicContext()) {
    return;
  }

  $menu_title = menu_get_active_title();
  if (empty($menu_title)) {
    return;
  }

  // Add the title wothout a link.
  $active_trail[] = array(
    'title' => $menu_title,
    'href' => '<nolink>',
    'localized_options' => array(),
  );
}