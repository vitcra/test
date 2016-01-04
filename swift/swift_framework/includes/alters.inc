<?php

/**
 * Implements hook_menu_alter
 *
 * Check if parent has changed or not and capture it
 * If yes, we need to re-compute url aliases
 * That will be done later in various menu_link_ hooks
 */
function swift_framework_menu_link_alter(&$item) {
  if (!isset($item['mlid']) || empty($item['menu_name'])) {
    return;
  }

  $existing_item = db_query('SELECT * FROM {menu_links} WHERE mlid = :mlid', array('mlid' => $item['mlid']))->fetchAssoc();

  if (empty($existing_item) || $item['plid'] != $existing_item['plid'] || $item['link_title'] != $existing_item['link_title'] || $item['link_path'] != $existing_item['link_path']) {

    // existing item has changed, but are there other items pointing to same node?
    // if yes, by default we give priority to the current item, i.e.
    // assuming Automatic path alias is on, the path will be rewritten
    // in accordance with the last changed item that points to same node
    // exception: if current item is not in main menu, but there exists another item in main menu
    // than do not update path

    if ($item['menu_name'] !== MASTER_MENU) {
      $other_main_menu_item = db_query('SELECT * FROM {menu_links} WHERE mlid != :mlid AND menu_name = :menuname AND link_path = :path', array('mlid' => $item['mlid'], ':menuname' => MASTER_MENU, ':path' => $item['link_path']))->fetchAssoc();

      if (!empty($other_main_menu_item['mlid'])) {
        return;
      }
    }

    $item['swift_framework_parent_changed'] = TRUE;
  }
}

/**
 * Implements hook_menu_link_insert
 */
function swift_framework_menu_link_insert($item) {
  _swift_framework_menu_link_update_process($item);
}

/**
 * Implements hook_menu_link_update
 */
function swift_framework_menu_link_update($item) {
  _swift_framework_menu_link_update_process($item);
}

/**
 * Implements hook_menu_link_delete
 */
// for now leave it, pathauto does not work anyway
/*function swift_framework_menu_link_delete($item) {
  $item['swift_framework_parent_changed'] = TRUE;
  _swift_framework_menu_link_update_process($item);
}*/

function _swift_framework_menu_link_update_process($item) {
  if (empty($item['swift_framework_parent_changed'])) {
    return;
  }

  // get the children, including this same item
  $children = db_select('menu_links', 'ml')
    ->fields('ml')
    ->condition('p'. $item['depth'], $item['mlid'])
    ->orderBy('depth')
    ->execute()
    ->fetchAllAssoc('mlid', PDO::FETCH_ASSOC);

  foreach ($children as $link) {
    $nid = (int) str_replace('node/', '', $link['link_path']);
    if (!$nid) {
      continue;
    }

    // Update the URL alias for this node
    swift_framework_node_url_alias_update($nid, $link);

    // Check if it has translations and update those as well
    $translations = translation_node_get_translations($nid);
    foreach ($translations as $translation) {
      swift_framework_node_url_alias_update($translation->nid);
    }

    // See if this is a topic and get all the children in the topic menu
    $topic_menu = swift_framework_topic_menu_from_node($nid);
    if (!$topic_menu) {
      continue;
    }

    $topic_children = db_select('swift_menu_links', 'sml')
      ->fields('sml')
      ->condition('menu_name', 'topic_'. $nid)
      ->orderBy('depth')
      ->execute()
      ->fetchAllAssoc('mlid', PDO::FETCH_ASSOC);

    foreach ($topic_children as $link) {
      $nid = (int) str_replace('node/', '', $link['link_path']);
      if (!$nid) {
        continue;
      }

      // Update the URL alias for this node
      swift_framework_node_url_alias_update($nid);

      // Check if it has translations and update those as well
      $translations = translation_node_get_translations($nid);
      foreach ($translations as $translation) {
        swift_framework_node_url_alias_update($translation->nid);
      }
    }
  }
}

function swift_framework_node_url_alias_update($node, $link = array()) {
  if (is_numeric($node)) {
    $node = node_load($node);
  }
  if (empty($node->menu)) {
    $node->menu = $link;
  }
  // largely copied from pathauto node update alias
  // we do not call it directly because we want to force alias update
  $options = array('language' => pathauto_entity_language('node', $node));

  // Skip processing if the node has no pattern.
  if (!pathauto_pattern_load_by_entity('node', $node->type, $options['language'])) {
    return;
  }

  module_load_include('inc', 'pathauto');
  $uri = entity_uri('node', $node);
  pathauto_create_alias('node', 'bulkupdate', $uri['path'], array('node' => $node), $node->type, $options['language']);
}

/**
 * Implements hook_form_BASE_FORM_ID_alter().
 */
function swift_framework_form_menu_edit_item_alter(&$form, $form_state) {
  $form['#validate'][] = 'swift_framework_menu_edit_item_validate';
}

function swift_framework_menu_edit_item_validate($form, &$form_state) {
  if (!empty($form_state['triggering_element']['#submit']) &&in_array('menu_item_delete_submit', $form_state['triggering_element']['#submit'])) {
    return;
  }

  /*list($menu_name, $mlid) = explode(':', $form_state['values']['parent'], 2);
  if (empty($menu_name) || $menu_name !== MASTER_MENU) {
    return;
  }*/

  $nid = (int) str_replace('node/', '', $form_state['values']['link_path']);
  if ($nid && $topic_menu = swift_framework_topic_menu_from_node($nid) && !empty($topic_menu->child_link)) {
    form_set_error('link_path', t('The path %path refers to a topic child page and cannot be added to the main menu', array('%path' => $form_state['values']['link_path'])));
  }
}


/**
 * Implements hook_form_BASE_FORM_ID_alter().
 */
function swift_framework_form_node_form_alter(&$form, $form_state) {
  $node = $form['#node'];
  $default_language = language_default('language');

  //set default language to site default language when creating new nodes
  if (empty($node->nid) && i18n_node_type_enabled($node) && empty($form_state['node']->translation_source) && !empty($form['language']['#options'])) {
    $form['language']['#default_value'] = $default_language;
  }
  // call a 'private' implemention of swift_framework_form_node_form_alter()
  // see i18n_node_form_node_form_alter
  $form['#after_build'][] = '_swift_framework_form_node_form_after_build';

  // alter code that relates to the menu settings
  _swift_framework_form_node_menu_alter($form, $form_state);
}

/**
 * Called by swift_framework_form_node_form_alter
 */
function _swift_framework_form_node_form_after_build($form, &$form_state) {
  $node = $form['#node'];

  // SDC-903
  // Allow all languages for these specific node bundles.
  $exceptions = array(
    'topic_language_landing',
    'resource',
    'resource_center',
  );

  // only allow default language and language_none for new nodes
  if (!in_array($node->type, $exceptions) && empty($node->nid) && i18n_node_type_enabled($node) && empty($form_state['values']['translation_source']) && !empty($form['language']['#options'])) {
    $default_language = language_default('language');
    foreach ($form['language']['#options'] as $k => $v) {
      if ($k != LANGUAGE_NONE && $k != $default_language) {
        unset($form['language']['#options'][$k]);
      }
    }
  }
  // if node is language neutral allow language change only to English
  // but check first if it has no topic child pages
  if (!empty($node->nid) && i18n_node_type_enabled($node) && empty($form_state['values']['translation_source']) && !empty($form['language']['#options']) && $form['language']['#value'] === LANGUAGE_NONE) {

    $contentNavigation = ContentNavigation::getInstance();
    $currentPage = $contentNavigation->getCurrentPage();
    $children = $currentPage->getTopicLinks();
    $default_language = language_default('language');
    if (empty($children)) {
      foreach ($form['language']['#options'] as $k => $v) {
        if ($k != LANGUAGE_NONE && $k != $default_language) {
          unset($form['language']['#options'][$k]);
        }
      }
    }
    else {
      foreach ($form['language']['#options'] as $k => $v) {
        if ($k != LANGUAGE_NONE) {
          unset($form['language']['#options'][$k]);
        }
      }
    }
  }

  if (isset($form['translation']['#weight'])) {
    $form['translation']['#weight'] = isset($form['additional_settings']['#weight']) ? $form['additional_settings']['#weight'] - 1 : 100;
  }
  return $form;
}

/**
 * See swift_framework_form_node_form_alter().
 *
 * Adds Topic Menu vertical tab and logic
 * follow comments inside function
 *
 *
 * @param $form
 * @param $form_state
 */
function _swift_framework_form_node_menu_alter(&$form, $form_state) {
  $node = $form['#node'];
  $default_language = language_default('language');

  // Add child page on a given topic
  // save topic_id for unwanted ajax calls
  // Media Browser for example will lose the $_GET
  if (!empty($_REQUEST['topic_id'])) {
    $form['topic_id'] = array(
      '#type' => 'hidden',
      '#value' => $_REQUEST['topic_id'],
    );
  }

  // menu settings only for source nodes
  $language_neutral_topics = FALSE;
  if (!empty($node->language) && $node->language !== $default_language && $node->language !== LANGUAGE_NONE) {
    unset($form['menu']);
    $language_neutral_topics = TRUE;
  }

  if (!empty($_REQUEST['topic_id']) && isset($form['menu'])) {
    unset($form['menu']);
  }

  if (!user_access('administer swift menu')) {
    return;
  }

  // topic menu form is relevant only for potential topic children
  if (empty($node->type) || !swift_framework_type_is('topic_child', $node->type)) {
    return;
  }

  // skip if node already has translations in a topic menu
  if (swift_framework_topic_child_translations($node)) {
    return;
  }

  // some child types have special treatment when they are _added_
  // see swift_ordering_add_dispatch_to_az_menu()
  // so no need for a topic menu in this case
  $automatic_menu_types = array('dispatch_service', 'dispatch_product');
  if (empty($node->nid) && in_array($node->type, $automatic_menu_types)) {
    return;
  }

  if (!empty($node->nid)) {
    // This is a translation of a child page that is already in the menu
    // exit, as only source nodes go to topic menus
    $topic_menu_item = swift_framework_topic_menu_link_load('node/'. $node->nid);
    if (!$topic_menu_item && !empty($node->tnid) && $node->nid != $node->tnid) {
      $topic_menu_item = swift_framework_topic_menu_link_load('node/'. $node->tnid);
      if ($topic_menu_item) {
        return;
      }
    }
  }
  elseif (!empty($_REQUEST['translation'])) {
    $form['translation'] = array(
      '#type' => 'hidden',
      '#value' => $_REQUEST['translation'],
    );
    // This is a new translation being created
    // Check the if source node is already part of the a topic menu
    // or any of the existing translations of the source node is already part of the menu
    // exit, as all translations always go together in one topic menu
    $source = node_load(swift_framework_get_tnid($_REQUEST['translation']));
    if (swift_framework_topic_child_translations($source, TRUE)) {
      return;
    }
  }

  // set some default values when the context demands it
  if (!isset($topic_menu_item['mlid'])) {
    $topic_menu_item = array('topic_id' => 0, 'mlid' => 0, 'plid' => 0, 'position' => 0);
    // here we are adding a child via 'Add child' tab on a topic node
    if (!empty($_REQUEST['topic_id'])) {
      $topic_menu = swift_framework_topic_menu_load('topic_'. $_REQUEST['topic_id']);
      if ($topic_menu) {
        $topic_menu_item['topic_id'] = $topic_menu['nid'];
        $topic_menu_item['topic_title'] = $topic_menu['title'];
      }
    }
  }
  else {
    $topic_menu_item['position'] = swift_framework_topic_menu_link_before($topic_menu_item);
  }

  // finally build the topic menu form elements
  $form['topic_menu'] = array(
    '#type' => 'fieldset',
    '#title' => t('Topic menu'),
    '#tree' => TRUE,
    '#group' => 'additional_settings',
  );

  $form['topic_menu']['item'] = array(
    '#type' => 'container',
    '#prefix' => '<div id="select-topic-menu">',
    '#suffix' => '</div>',
  );

  if (empty($topic_menu_item['topic_id'])) {
    $topic_options = swift_framework_topic_menu_options($language_neutral_topics);
  }
  else {
    $topic_options = array($topic_menu_item['topic_id'] => $topic_menu_item['topic_title']);
  }
  $topic_options = array(t('- none -')) + $topic_options;
  $topic_selected = isset($form_state['values']['topic_menu']['item']['topic_id']) ?
    $form_state['values']['topic_menu']['item']['topic_id'] : $topic_menu_item['topic_id'];

  $form['topic_menu']['item']['topic_id'] = array(
    '#type' => 'select',
    '#title' => t('Topic'),
    '#options' => $topic_options,
    '#default_value' => $topic_selected,
    '#ajax' => array(
      'callback' => '_swift_framework_topic_menu_item_ajax_callback',
      'wrapper' => 'select-topic-menu',
    ),
  );

  if ($topic_menu_item['mlid'] && swift_framework_topic_child_children($topic_menu_item['mlid'])) {
    $parent_options = array();
  }
  else {
    $item = array('menu_name' => 'topic_' . $topic_selected, 'mlid' => $topic_menu_item['mlid']);
    $parent_options = swift_framework_topic_menu_parent_options($item);
  }

  $parent_options = array(t('- none -')) + $parent_options;
  $parent_selected = isset($form_state['values']['topic_menu']['item']['plid']) ? $form_state['values']['topic_menu']['item']['plid'] : $topic_menu_item['plid'];

  $form['topic_menu']['item']['plid'] = array(
    '#type' => 'select',
    '#title' => t('Parent'),
    '#options' => $parent_options,
    '#default_value' => $parent_selected,
    '#ajax' => array(
      'callback' => '_swift_framework_topic_menu_item_ajax_callback',
      'wrapper' => 'select-topic-menu',
    ),
  );

  $item = array('menu_name' => 'topic_' . $topic_selected, 'mlid' => $topic_menu_item['mlid'], 'plid' => $parent_selected);
  $position_options = swift_framework_topic_menu_sibling_options($item);
  foreach ($position_options as $key => $option) {
    $position_options[$key] = t('After') .': '. $option;
  }
  $position_options = array(t('First one')) + $position_options;
  // position last if it is a new child page
  if (empty($topic_menu_item['mlid']) && isset($key)) {
    $topic_menu_item['position'] = $key;
  }

  $position_selected = isset($form_state['values']['topic_menu']['item']['position']) ? $form_state['values']['topic_menu']['item']['position'] : $topic_menu_item['position'];

  $form['topic_menu']['item']['position'] = array(
    '#type' => 'select',
    '#title' => t('Position'),
    '#options' => $position_options,
    '#default_value' => $position_selected,
  );

  $form['#original_topic_menu_item'] = $topic_menu_item;
  $form['#validate'][] = '_swift_framework_form_node_menu_validate';
  //$form['#submit'][] = '_swift_framework_form_node_menu_submit';
  // this submit is called later, as we need, the one above is not so good
  // we will lose our alias medling otherwise
  $form['actions']['submit']['#submit'][] = '_swift_framework_form_node_menu_submit';
}

function _swift_framework_topic_menu_item_ajax_callback($form, $form_state) {
  return $form['topic_menu']['item'];
}

function _swift_framework_form_node_menu_validate($form, &$form_state) {

  if ($form_state['triggering_element']['#type'] === 'select' || empty($form_state['values']['topic_menu']['item']['topic_id'])) {
    return;
  }

  if (!empty($form_state['values']['menu']['enabled']) && !empty($form_state['values']['topic_menu']['item']['topic_id'])) {
    form_set_error('topic_menu', t('This page cannot be part of both main menu and topic menu.'));
  }
  if (isset($form['field_tab_title']) && empty($form_state['values']['field_tab_title'][LANGUAGE_NONE][0]['value']) && !empty($form_state['values']['topic_menu']['item']['topic_id'])) {
    form_set_error('field_tab_title', t('Child page tab title is required if you want to add this page to a topic.'));
  }

  $node = $form['#node'];
  $topic_menu = swift_framework_topic_menu_load('topic_'. $form_state['values']['topic_menu']['item']['topic_id']);

  // does this node has translations that are part of the menu?
  if ($topic_menu['language'] != LANGUAGE_NONE && swift_framework_topic_child_translations($node)) {
    form_set_error('topic_menu', t('This page has at least one translation that is a child page. Hence it can be attached only to a language neutral topic.'));
  }

}

function _swift_framework_form_node_menu_submit($form, &$form_state) {
  if (empty($form_state['values']['topic_menu']['item']['topic_id']) && empty($form['#original_topic_menu_item']['mlid'])) {
    return;
  }

  if (!empty($form_state['values']['field_tab_title'][LANGUAGE_NONE][0]['value'])) {
    $link_title = $form_state['values']['field_tab_title'][LANGUAGE_NONE][0]['value'];
  }
  else {
    $link_title = $form_state['node']->title;
  }

  $item = array(
    'link_title' => $link_title,
    'topic_id' => $form_state['values']['topic_menu']['item']['topic_id'],
    'plid' => $form_state['values']['topic_menu']['item']['plid'],
    'position' => $form_state['values']['topic_menu']['item']['position'],
  );
  $original_item = $form['#original_topic_menu_item'];
  $changed = FALSE;
  foreach ($item as $key => $value) {
    if ($key === 'position') {
      continue;
    }
    if (!isset($original_item[$key]) || $original_item[$key] != $value) {
      $changed = TRUE;
      break;
    }
  }

  $item['menu_name'] = 'topic_'. $item['topic_id'];
  if (!empty($original_item['mlid'])) {
    $item['mlid'] = $original_item['mlid'];
  }

  if ($changed) {
    if (empty($item['topic_id'])) {
      _swift_framework_topic_menu_delete_item($item);
      if (!empty($form_state['node']->nid) && empty($form_state['values']['menu']['enabled']) && !empty($form_state['node']->path['pid'])) {
        // delete the path alias
        path_delete($form_state['node']->path['pid']);
        //unset($form_state['node']->path);
      }
    }
    elseif (!empty($form_state['node']->nid)) {
      $nid = $form_state['node']->nid;
      $item['link_path'] = 'node/'. $nid;
      _swift_framework_topic_menu_link_save($item, $original_item);
    }
  }

  // update the weights
  if (empty($original_item['mlid']) || $original_item['position'] != $item['position']) {
    $siblings = swift_framework_topic_menu_sibling_options($item);
    if (count($siblings)) {
      if ($item['position']) { // our item is after another one
        $weight = 0;
        foreach (array_keys($siblings) as $key) {
          $siblings[$key] = $weight++;
          if ($item['position'] == $key) {
            $siblings[$item['mlid']] = $weight++;
          }
        }
      }
      else { // out item is the first one
        $weight = 1;
        foreach (array_keys($siblings) as $key) {
          $siblings[$key] = $weight++;
        }
        $siblings[$item['mlid']] = 0;
      }
      foreach ($siblings as $mlid => $weight) {
        db_update('swift_menu_links')->fields(array('weight' => $weight))->condition('mlid', $mlid)->execute();
      }
    }
  }
}

/**
 * Implements hook_pathauto_alias_alter().
 *
 * Altering the alias for translated nodes to prepend their language code!
 */
function swift_framework_pathauto_alias_alter(&$alias, &$context) {
  $language = $context['language'];
  if ($language === 'en' || $language === LANGUAGE_NONE || empty($alias)) {
    return;
  }

  $alias = $language . '/' . $alias;
}

