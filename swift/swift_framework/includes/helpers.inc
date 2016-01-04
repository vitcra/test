<?php

/**
 * @file
 * Helper functions for the Swift framework.
 */

function _swift_framework_load_topic_menu($menu_name) {
  $topic_menu = db_select('swift_menu', 'sm')
    ->fields('sm')
    ->condition('menu_name', $menu_name)
    ->execute()
    ->fetchObject();

  return $topic_menu;
}

/**
 * Create a menu for the central topic.
 * @param stdClass $node
 */
function _swift_framework_create_menu($node) {

  // source language is no longer guaranteed to be the default language
  // change introduced by SDC-903
  if (!empty($node->tnid) && (empty($node->nid) || $node->tnid <> $node->nid)) {
    return;
  }
  else if (!empty($node->translation_source)) {
    return;
  }

  // Skip for all non-topics.
  $contentNavigation = ContentNavigation::getInstance();
  $currentPage = $contentNavigation->getCurrentPage();

  if (!$currentPage) {
    return;
  }

  if (in_array($node->type, $currentPage->topicChildTypes, TRUE) && $title = _swift_framework_get_tab_title($node)) {
    // update menu item title for a child page that
    // provides a tab tile
    db_update('swift_menu_links')
      ->fields(array('link_title' => $title))
      ->condition('link_path', 'node/'. $node->nid)
      ->execute();
  }

  if (!in_array($node->type, $currentPage->topicTypes, TRUE)) {
    return;
  }

  // Create the custom Swift menu.
  $record = new stdClass();
  $record->menu_name = 'topic_' . $node->nid;
  $record->title = $node->title;
  $record->description = t('Menu for %title', array('%title' => 'Topic '. $node->nid));
  $record->nid = $node->nid;
  $record->language = $node->language;

  $topic_menu = _swift_framework_load_topic_menu($record->menu_name);

  if (!empty($topic_menu->menu_name)) {
    drupal_write_record('swift_menu', $record, array('menu_name'));
  }
  else {
    drupal_write_record('swift_menu', $record);
  }

}

/**
 * Removes a Swift menu and its links.
 * @param type $node
 */
function _swift_framework_remove_menu($node) {

  $menu_name = 'topic_' . $node->nid;
  db_query('DELETE FROM {swift_menu} WHERE menu_name = :name', array(':name' => $menu_name));
  db_query('DELETE FROM {swift_menu_links} WHERE menu_name = :name', array(':name' => $menu_name));
  db_query('DELETE FROM {swift_menu_links} WHERE link_path = :path', array(':path' => 'node/'. $node->nid));
}

/**
 * Saves a menu link.
 *
 * After calling this function, rebuild the menu cache using
 * menu_cache_clear_all().
 *
 * @param $item
 *   An associative array representing a menu link item, with elements:
 *   - link_path: (required) The path of the menu item, which should be
 *     normalized first by calling drupal_get_normal_path() on it.
 *   - link_title: (required) Title to appear in menu for the link.
 *   - menu_name: (optional) The machine name of the menu for the link.
 *     Defaults to 'navigation'.
 *   - weight: (optional) Integer to determine position in menu. Default is 0.
 *   - mlid: (optional) Menu link identifier, the primary integer key for each
 *     menu link. Can be set to an existing value, or to 0 or NULL
 *     to insert a new link.
 *   - plid: (optional) The mlid of the parent.
 *
 * @return
 *   The mlid of the saved menu link, or FALSE if the menu link could not be
 *   saved.
 */
function _swift_framework_topic_menu_link_save(&$item, $existing_item = array(), $parent_candidates = array()) {

  // Load defaults.
  $item += array(
    'menu_name' => '',
    'weight' => 0,
    'link_title' => '',
    'p1' => NULL,
    'p2' => NULL,
  );
  if (isset($item['mlid'])) {
    if (!$existing_item) {
      $existing_item = db_query('SELECT * FROM {swift_menu_links} WHERE mlid = :mlid', array('mlid' => $item['mlid']))->fetchAssoc();
    }
  }
  else {
    $existing_item = FALSE;
  }

  // Try to find a parent link. If found, assign it and derive its menu.
  $parent = _swift_framework_menu_link_find_parent($item, $parent_candidates);
  if (!empty($parent['mlid'])) {
    $item['plid'] = $parent['mlid'];
    $item['menu_name'] = $parent['menu_name'];
  }
  // If no corresponding parent link was found, move the link to the top-level.
  else {
    $item['plid'] = 0;
  }

  if (empty($item['plid'])) {
    $item['plid'] = 0;
  }
  $menu_name = $item['menu_name'];

  if (!$existing_item) {
    $item['mlid'] = db_insert('swift_menu_links')
      ->fields(array(
        'menu_name' => $item['menu_name'],
        'plid' => $item['plid'],
        'link_path' => $item['link_path'],
        'link_title' => $item['link_title'],
        'weight' => $item['weight'],
        //'depth' => $item['depth'],
      ))
      ->execute();
  }

  // Directly fill parents for top-level links.
  if ($item['plid'] == 0) {
    $item['p1'] = $item['mlid'];
    for ($i = 2; $i <= 2; $i++) {
      $item["p$i"] = 0;
    }
    $item['depth'] = 1;
  }
  else {
    $item['depth'] = 2;

    $i = 1;
    while ($i < $item['depth']) {
      $p = 'p' . $i++;
      $item[$p] = $parent[$p];
    }
    $p = 'p' . $i++;
    // The parent (p1 - p9) corresponding to the depth always equals the mlid.
    $item[$p] = $item['mlid'];
    while ($i <= 2) {
      $p = 'p' . $i++;
      $item[$p] = 0;
    }

  }

  if (!$existing_item || (array_intersect_key($item, $existing_item) != $existing_item)) {
    db_update('swift_menu_links')
      ->fields(array(
        'menu_name' => $item['menu_name'],
        'plid' => $item['plid'],
        'link_path' => $item['link_path'],
        'link_title' => $item['link_title'],
        'weight' => $item['weight'],
        'depth' => $item['depth'],
        'p1' => $item['p1'],
        'p2' => $item['p2'],
      ))
      ->condition('mlid', $item['mlid'])
      ->execute();

    menu_cache_clear($menu_name);
    if ($existing_item && $menu_name != $existing_item['menu_name']) {
      menu_cache_clear($existing_item['menu_name']);
    }

    // trigger update of url aliases
    $nid = (int) str_replace('node/', '', $item['link_path']);
    if ($nid && (empty($existing_item) || $item['plid'] != $existing_item['plid'])) {
      swift_framework_node_url_alias_update($nid);
    }
  }
  return $item['mlid'];

}

/**
 * Load a swift menu.
 */
function swift_framework_topic_menu_load($menu_name) {
  $menu = db_select('swift_menu', 'sm')->fields('sm')->condition('menu_name', $menu_name)->execute()->fetchAssoc();
  return empty($menu) ? FALSE : $menu;
}

/**
 * Load a swift menu link by mlid or link_path.
 */
function swift_framework_topic_menu_link_load($mlid) {
  $query = db_select('swift_menu_links', 'ml');
  $query->fields('ml');
  $query->join('swift_menu', 'sm', 'ml.menu_name = sm.menu_name');
  $query->addField('sm', 'language', 'topic_language');
  $query->addField('sm', 'nid', 'topic_id');
  $query->addField('sm', 'title', 'topic_title');

  if (is_numeric($mlid)) {
    $query->condition('ml.mlid', $mlid);
  }
  else {
    $query->condition('ml.link_path', $mlid);
  }

  if ($item = $query->execute()->fetchAssoc()) {
    return $item;
  }

  return FALSE;
}


/**
 * Delete one or several menu links.
 *
 * @param $mlid
 *   A valid menu link mlid or NULL. If NULL, $path is used.
 * @param $path
 *   The path to the menu items to be deleted. $mlid must be NULL.
 */
function _swift_framework_menu_link_delete($mlid, $path = NULL) {
  if (isset($mlid)) {
    _swift_framework_topic_menu_delete_item(db_query("SELECT * FROM {swift_menu_links} WHERE mlid = :mlid", array(':mlid' => $mlid))->fetchAssoc());
  }
  else {
    $result = db_query("SELECT * FROM {swift_menu_links} WHERE link_path = :link_path", array(':link_path' => $path));
    foreach ($result as $link) {
      _swift_framework_topic_menu_delete_item($link);
    }
  }
}

/**
 * Deletes a single menu link.
 *
 * @param $item
 *   Item to be deleted.
 * @param $force
 *   Forces deletion. Internal use only, setting to TRUE is discouraged.
 *
 * @see menu_link_delete()
 */
function _swift_framework_topic_menu_delete_item($item, $force = FALSE) {
  $item = is_object($item) ? get_object_vars($item) : $item;
  if ($item) {
    // Children get re-attached to the item's parent.
    $result = db_query("SELECT mlid FROM {swift_menu_links} WHERE plid = :plid", array(':plid' => $item['mlid']));
    foreach ($result as $m) {
      $child = swift_framework_topic_menu_link_load($m->mlid);
      $child['plid'] = $item['plid'];
      _swift_framework_topic_menu_link_save($child);
    }

    db_delete('swift_menu_links')->condition('mlid', $item['mlid'])->execute();

  }
}

/**
 * Load all custom menu data.
 *
 * @return
 *   Array of custom menu data.
 */
function _swift_framework_menu_load_all() {
  $custom_menus = &drupal_static(__FUNCTION__);
  if (!isset($custom_menus)) {
    if ($cached = cache_get('swift_menu', 'cache_menu')) {
      $custom_menus = $cached->data;
    }
    else {
      $custom_menus = db_query('SELECT * FROM {swift_menu}')->fetchAllAssoc('menu_name', PDO::FETCH_ASSOC);
      cache_set('swift_menu', $custom_menus, 'cache_menu');
    }
  }
  return $custom_menus;
}

/**
 * Return topic menus as options for select element
 * Menu title, keyed by topic id
 *
 * @return array
 */
function swift_framework_topic_menu_options($language_neutral = FALSE) {
  $query = db_select('swift_menu', 'sm')->fields('sm', array('nid'));
  $query->addExpression("CONCAT(title, ' (', language, ')')", 'title_');
  $query->orderBy('title');

  if ($language_neutral) {
    $query->condition('language', LANGUAGE_NONE);
  }

  $options = $query->execute()->fetchAllKeyed();

  return is_array($options) ? $options : array();
}

/**
 * Get the options for a topic menu link as available parents.
 * Only first level
 */
function swift_framework_topic_menu_parent_options($item) {

  $options = db_select('swift_menu_links', 'sml')
    ->fields('sml', array('mlid', 'link_title'))
    ->condition('menu_name', $item['menu_name'])
    ->condition('plid', 0)
    ->orderBy('weight')
    ->orderBy('p1')
    ->orderBy('p2')
    ->orderBy('link_title')
    ->execute()
    ->fetchAllKeyed();

  // cannot be parent to itself
  if (isset($item['mlid']) && !empty($options[$item['mlid']])) {
    unset($options[$item['mlid']]);
  }

  return is_array($options) ? $options : array();
}

function swift_framework_topic_menu_sibling_options($item) {

  if (!isset($item['plid'])) {
    $item['plid'] = 0;
  }

  $options = db_select('swift_menu_links', 'sml')
    ->fields('sml', array('mlid', 'link_title'))
    ->condition('menu_name', $item['menu_name'])
    ->condition('plid', $item['plid'])
    ->orderBy('weight')
    ->orderBy('p1')
    ->orderBy('p2')
    ->orderBy('link_title')
    ->execute()
    ->fetchAllKeyed();

  // cannot be sibling to itself
  if (!empty($options[$item['mlid']])) {
    unset($options[$item['mlid']]);
  }

  return is_array($options) ? $options : array();
}

/**
 * Returns the link that comes before $item or 0
 */
function swift_framework_topic_menu_link_before($item) {
  $links = db_select('swift_menu_links', 'sml')
    ->fields('sml', array('mlid'))
    ->condition('menu_name', $item['menu_name'])
    ->condition('plid', $item['plid'])
    ->orderBy('weight')
    ->orderBy('p1')
    ->orderBy('p2')
    ->orderBy('link_title')
    ->execute();

  $before = 0;
  foreach ($links as $link) {
    if ($link->mlid == $item['mlid']) {
      return $before;
    }
    $before = $link->mlid;
  }
  return 0;
}

/**
 * Finds a possible parent for a given menu link.
 *
 * Because the parent of a given link might not exist anymore in the database,
 * we apply a set of heuristics to determine a proper parent:
 *
 *  - use the passed parent link if specified and existing.
 *  - else, use the first existing link down the previous link hierarchy
 *  - else, for system menu links (derived from hook_menu()), reparent
 *    based on the path hierarchy.
 *
 * @param $menu_link
 *   A menu link.
 * @param $parent_candidates
 *   An array of menu links keyed by mlid.
 *
 * @return
 *   A menu link structure of the possible parent or FALSE if no valid parent
 *   has been found.
 */
function _swift_framework_menu_link_find_parent($menu_link, $parent_candidates = array()) {
  $parent = FALSE;

  // This item is explicitely top-level, skip the rest of the parenting.
  if (isset($menu_link['plid']) && empty($menu_link['plid'])) {
    return $parent;
  }

  // If we have a parent link ID, try to use that.
  $candidates = array();
  if (isset($menu_link['plid'])) {
    $candidates[] = $menu_link['plid'];
  }

  // Else, if we have a link hierarchy try to find a valid parent in there.
  if (!empty($menu_link['depth']) && $menu_link['depth'] > 1) {
    for ($depth = $menu_link['depth'] - 1; $depth >= 1; $depth--) {
      $candidates[] = $menu_link['p' . $depth];
    }
  }

  foreach ($candidates as $mlid) {
    if (isset($parent_candidates[$mlid])) {
      $parent = $parent_candidates[$mlid];
    }
    else {
      $parent = db_query("SELECT * FROM {swift_menu_links} WHERE mlid = :mlid", array(':mlid' => $mlid))->fetchAssoc();
    }
    if ($parent) {
      return $parent;
    }
  }

  // If everything else failed, try to derive the parent from the path
  // hierarchy. This only makes sense for links derived from menu router
  // items (ie. from hook_menu()).
  $query = db_select('swift_menu_links');
  $query->condition('module', 'system');
  // We always respect the link's 'menu_name'; inheritance for router items is
  // ensured in _menu_router_build().
  $query->condition('menu_name', $menu_link['menu_name']);

  // Find the parent - it must be unique.
  $parent_path = $menu_link['link_path'];
  do {
    $parent = FALSE;
    $parent_path = substr($parent_path, 0, strrpos($parent_path, '/'));
    $new_query = clone $query;
    $new_query->condition('link_path', $parent_path);
    // Only valid if we get a unique result.
    if ($new_query->countQuery()->execute()->fetchField() == 1) {
      $parent = $new_query->fields('menu_links')->execute()->fetchAssoc();
    }
  } while ($parent === FALSE && $parent_path);

  return $parent;
}

function _swift_framework_get_translation($nid, $language) {
  $translation_nid = db_select('node', 'n')
    ->fields('n', array('nid'))
    ->condition('tnid', $nid)
    ->condition('language', $language)
    ->execute()
    ->fetchField();
  return $translation_nid ? $translation_nid : $nid;
}

function swift_framework_get_topic_languages($topic) {
  if (is_numeric($topic)) {
    $topic = node_load($topic);
  }

  $default = array(LANGUAGE_NONE => LANGUAGE_NONE);
  if ($topic->language === LANGUAGE_NONE) {
    return $default;
  }

  $translations = translation_node_get_translations($topic->tnid);
  if ($translations) {
    return drupal_map_assoc(array_keys($translations)) + $default;
  }
  else {
    return array($topic->language => $topic->language) + $default;
  }

}

function _swift_framework_get_tab_title($node, $field_name = 'field_tab_title') {
  if (is_numeric($node)) {
    $node = node_load($node);
    if (!$node) {
      return '';
    }
  }
  $wrapped_node = entity_metadata_wrapper('node', $node);
  if (isset($wrapped_node->$field_name)) {
    return $wrapped_node->$field_name->value();
  }
  return '';
}

/**
 * Check if the node is related to a topic menu
 * If yes, return the menu, otherwise return FALSE
 */
function swift_framework_topic_menu_from_node($node) {

  if (is_numeric($node)) {
    $node = node_load($node);
  }

  $topic_menu = FALSE;

  $tnid = empty($node->tnid) ? $node->nid : $node->tnid;

  if (swift_framework_type_is('topic', $node->type)) {
    $topic_menu = _swift_framework_load_topic_menu('topic_'. $tnid);
  }
  else {
    // look up in the swift menus
    $menu_link = swift_framework_topic_menu_link_load('node/'. $tnid);

    // we looked up the source, but maybe a language neutral topic is in play here
    if (!$menu_link && $tnid != $node->nid) {
      $menu_link = swift_framework_topic_menu_link_load('node/'. $node->nid);
    }

    if (!empty($menu_link['menu_name'])) {
      $topic_menu = _swift_framework_load_topic_menu($menu_link['menu_name']);
      $topic_menu->child_link = $menu_link;
    }
  }

  return $topic_menu;
}

function swift_framework_get_topic_link($topic_menu) {
  if (!empty($topic_menu->nid)) {
    $topic_link = db_select('menu_links', 'ml')->fields('ml')->condition('link_path', 'node/'. $topic_menu->nid)->execute()->fetchObject();
    return empty($topic_link) ? FALSE : $topic_link;
  }
  return FALSE;
}

function swift_framework_type_is($category, $type) {
  static $category_types;

  if (!isset($category_types[$category]) || !is_array($category_types[$category])) {
    $category_types[$category] = variable_get('swift_'. $category .'_types', array());
  }
  return in_array($type, $category_types[$category], TRUE);
}

/**
 * Return translation set node id
 * If that is empty, returns same nid back
 */
function swift_framework_get_tnid($nid) {
  $tnid = db_select('node', 'n')->fields('n', array('tnid'))->condition('nid', $nid)->execute()->fetchField();
  return $tnid ? $tnid : $nid;
}

function swift_framework_topic_child_children($mlid) {
  $children = db_select('swift_menu_links', 'sml')
    ->fields('sml')
    ->condition('mlid', $mlid, '<>')
    ->condition('p1', $mlid)
    ->execute()
    ->fetchAllAssoc('mlid', PDO::FETCH_ASSOC);

  return $children;
}

/**
 * Returns true if the node has _other_ translations that are part of topic menus
 * if $all is TRUE then check even the node itself
 */
function swift_framework_topic_child_translations($node, $all = FALSE) {
  if (!empty($node->tnid)) {
    $tnid = $node->tnid;
  }
  elseif (!empty($node->translation_source->nid)) {
    $tnid = $node->translation_source->nid;
    $topic_menu_link = swift_framework_topic_menu_link_load('node/' . $tnid);
    if ($topic_menu_link) {
      return TRUE;
    }
  }
  else {
    return FALSE;
  }

  $translations = translation_node_get_translations($tnid);
  foreach ($translations as $translation) {
    if ($all || empty($node->nid) || $translation->nid != $node->nid) {
      $topic_menu_link = swift_framework_topic_menu_link_load('node/' . $translation->nid);
      if ($topic_menu_link) {
        return TRUE;
      }
    }
  }
  return FALSE;
}

/**
 * Generate a path alias for the node based on its location in the main menu/topic menu
 */
function swift_framework_generate_menu_path($node) {

  // if we are dealing with a translation, load the original node and use that one
  $default_language = language_default('language');
  if ($node->language !== $default_language && $node->language !== LANGUAGE_NONE && $node->tnid) {
    $node = node_load($node->tnid);
  }

  if (empty($node->menu['menu_name'])) {
    $menu = db_select('menu_links', 'ml')->fields('ml')->condition('link_path', 'node/'. $node->nid)->execute()->fetchAssoc();
    if ($menu) {
      $node->menu = $menu;
    }
  }

  if (!empty($node->menu['menu_name']) && !empty($node->menu['mlid'])) {
    $parents = token_menu_link_load_all_parents($node->menu['mlid']);
    $parents += array($node->menu['mlid'] => $node->menu['link_title']);
  }
  elseif (swift_framework_type_is('topic_child', $node->type) && $topic_menu = swift_framework_topic_menu_from_node($node)) {
    $topic_link = swift_framework_get_topic_link($topic_menu);
    if ($topic_link) {
      $parents = token_menu_link_load_all_parents($topic_link->mlid);
      $parents += array($topic_link->mlid => $topic_link->link_title);
    }
    else {
      $parents = array();
    }
    if ($topic_menu->child_link['depth'] == 2) {
      $child1 = swift_framework_topic_menu_link_load($topic_menu->child_link['p1']);
      $parents += array($child1['mlid'] => $child1['link_title']);
    }
    $parents += array($topic_menu->child_link['mlid'] => $topic_menu->child_link['link_title']);
  }
  // SDC-1275 introduced menu position
  // take it into account
  elseif ($menu_position = _swift_framework_get_node_menu_position($node)) {
    $parents = token_menu_link_load_all_parents($menu_position->mlid);
    $parents += array('menu-position' => $node->title);
  }

  if (!empty($parents)) {
    return implode('/', array_map('swift_framework_sanitize_url', $parents));
  }
  return '';
}

function swift_framework_sanitize_url($string) {
  //Lower case everything
  $string = strtolower($string);
  //Make alphanumeric (removes all other characters)
  $string = preg_replace('/[^a-z0-9_\s-]/', '_', $string);
  //Convert whitespace to dashes
  $string = preg_replace('/\s/', '-', $string);
  //convert multiple underscores into one
  $string = preg_replace('/_+/','_', $string);
  //convert multiple dashes into one
  $string = preg_replace('/-+/','-', $string);
  // final clean up _-_, -_-
  $string = preg_replace('/-[_-]+/','-', $string);
  $string = preg_replace('/_[-_]+/','_', $string);

  return $string;
}

function swift_framework_get_topic_children_nids($topic_id) {
  $nids = array();
  $result = db_select('swift_menu_links', 'sml')
    ->fields('sml', array('link_path'))
    ->condition('menu_name', 'topic_'. $topic_id)
    ->execute();

  foreach ($result as $item) {
    $nid = (int) str_replace('node/', '', $item->link_path);
    if ($nid) {
      $nids[] = $nid;
    }
  }
  return $nids;
}

/**
 * Get the position of the node as determined by menu_position, if any
 *
 * @see menu_position_evaluate_rules
 *
 * @param $node
 * @return mixed
 */
function _swift_framework_get_node_menu_position($node) {

  $matched_positions = array();

  $context = array(
    'path' => 'node/'. $node->nid,
    'entity_type' => 'node',
    'bundle_name' => $node->type,
    'node' => $node,
  );

  // Retrieve the rules from the database. For speed, we don't call
  // menu_position_read_rules() and unserialize the conditions only if needed.
  $rules = db_query('SELECT * FROM {menu_position_rules} WHERE enabled = :enabled ORDER BY weight, rid', array(':enabled' => 1));

  // Retrieve the list of menus the path is already in.
  $menu_names = db_query('SELECT menu_name FROM {menu_links} WHERE link_path = :path', array(':path' => $context['path']))->fetchCol();

  // Examine each rule and check its conditions.
  foreach ($rules as $rule) {
    if (in_array($rule->menu_name, $menu_names)) {
      // If the page is already placed in the rule's menu, skip the rule.
      $rule_matches = FALSE;
    }
    else {
      // A rule with no conditions always matches.
      $rule_matches = TRUE;
      // Go through each condition, ANDing each result.
      $rule->conditions = unserialize($rule->conditions);
      foreach ($rule->conditions as $plugin => $variables) {
        // Add the current rule and node to the callback's variables.
        $variables['rule'] = $rule;
        $variables['context'] = $context;
        // Find the plugin's callback function.
        $callback = menu_position_get_condition_callback($plugin);
        if ($callback) {
          // Check if this condition matches.
          $rule_matches = $callback($variables);
        }
        else {
          // If the callback cannot be found, the condition has failed.
          $rule_matches = FALSE;
        }
        // No need to check other conditions if this condition failed.
        if (!$rule_matches) {
          break;
        }
      }
    }

    // Let other modules manipulate the rule.
    $set_breadcrumb = FALSE;
    drupal_alter('menu_position_rule', $rule, $context, $rule_matches, $set_breadcrumb);

    // We've found a matching rule.
    if ($rule_matches) {
      // Don't let other rules match against this rule's menu.
      $menu_names[] = $rule->menu_name;
      $matched_positions[$rule->menu_name] = $rule;
    }
  }

  if (!empty($matched_positions[MASTER_MENU])) {
    return $matched_positions[MASTER_MENU];
  }

  return reset($matched_positions);
}