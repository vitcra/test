<?php

/**
 * @file
 * Administrative page callbacks for menu module.
 */

/**
 * Menu callback which shows an overview page of all the custom menus and their descriptions.
 */
function swift_framework_menu_overview_page() {
  $result = db_query("SELECT * FROM {swift_menu} ORDER BY title", array(), array('fetch' => PDO::FETCH_ASSOC));
  $header = array(t('Title'), array('data' => t('Operations'), 'colspan' => '2'));
  $rows = array();
  foreach ($result as $menu) {
    if (!swift_framework_topic_menu_access($menu)) {
      continue;
    }
    $row = array(theme('menu_admin_overview', array('title' => $menu['title'], 'name' => $menu['menu_name'], 'description' => $menu['description'])));
    $row[] = array('data' => l(t('list links'), 'admin/structure/swift-menu/manage/' . $menu['menu_name']));
    $row[] = array('data' => l(t('add link'), 'admin/structure/swift-menu/manage/' . $menu['menu_name'] . '/add'));
    $rows[] = $row;
  }

  return theme('table', array('header' => $header, 'rows' => $rows));
}


/**
 * Form for editing an entire menu tree at once.
 *
 * Shows for one menu the menu links accessible to the current user and
 * relevant operations.
 */
function swift_framework_menu_overview_form($form, &$form_state, $menu) {

  $form['#attached']['css'] = array(drupal_get_path('module', 'menu') . '/menu.css');
  // based on the fact that we can have only two levels

  $level1 = db_query('SELECT ml.* FROM {swift_menu_links} ml WHERE ml.menu_name = :menu AND ml.depth = 1 ORDER BY ml.weight ASC', array(':menu' => $menu['menu_name']), array('fetch' => PDO::FETCH_ASSOC));

  $links = array();
  foreach ($level1 as $item) {
    $links[] = $item;
    $level2 = db_query('SELECT ml.* FROM {swift_menu_links} ml WHERE ml.menu_name = :menu AND ml.plid = :plid ORDER BY ml.weight ASC', array(':menu' => $menu['menu_name'], ':plid' => $item['mlid']), array('fetch' => PDO::FETCH_ASSOC));

    foreach ($level2 as $item) {
      $links[] = $item;
    }
  }

  $tree = menu_tree_data($links);

  $form = array_merge($form, _swift_framework_menu_overview_tree_form($tree));
  $form['#menu'] =  $menu;

  if (element_children($form)) {
    $form['actions'] = array('#type' => 'actions');
    $form['actions']['submit'] = array(
      '#type' => 'submit',
      '#value' => t('Save configuration'),
    );
  }
  else {
    $form['#empty_text'] = t('There are no menu links yet');
  }

  return $form;
}

/**
 * Returns HTML for the menu overview form into a table.
 *
 * @param $variables
 *   An associative array containing:
 *   - form: A render element representing the form.
 *
 * @ingroup themeable
 */
function theme_swift_framework_menu_overview_form($variables) {
  $form = $variables['form'];

  drupal_add_tabledrag('menu-overview', 'match', 'parent', 'menu-plid', 'menu-plid', 'menu-mlid', TRUE, 1);
  drupal_add_tabledrag('menu-overview', 'order', 'sibling', 'menu-weight');

  $header = array(
    t('Menu link'),
    t('Weight'),
  );
  $header[] = array('data' => t('Operations'), 'colspan' => '3');

  $rows = array();
  foreach (element_children($form) as $mlid) {
    if (isset($form[$mlid]['#item'])) {
      $element = &$form[$mlid];
      // Build a list of operations.
      $operations = array();
      foreach (element_children($element['operations']) as $op) {
        $operations[] = array(
          'data' => drupal_render($element['operations'][$op]),
          'class' => array('menu-operations')
        );
        while (count($operations) < 2) {
          $operations[] = '';
        }
      }

      // Add special classes to be used for tabledrag.js.
      $element['plid']['#attributes']['class'] = array('menu-plid');
      $element['mlid']['#attributes']['class'] = array('menu-mlid');
      $element['weight']['#attributes']['class'] = array('menu-weight');

      // Change the parent field to a hidden. This allows any value but hides the field.
      $element['plid']['#type'] = 'hidden';

      $row = array();
      $row[] = theme('indentation', array('size' => $element['#item']['depth'] - 1)) . drupal_render($element['title']);
      $row[] = drupal_render($element['weight']) . drupal_render($element['plid']) . drupal_render($element['mlid']);
      $row = array_merge($row, $operations);

      $row = array_merge(array('data' => $row), $element['#attributes']);
      $row['class'][] = 'draggable';
      $rows[] = $row;
    }
  }
  $output = '';
  if (empty($rows)) {
    $rows[] = array(array('data' => $form['#empty_text'], 'colspan' => '7'));
  }
  $output .= theme('table', array('header' => $header, 'rows' => $rows, 'attributes' => array('id' => 'menu-overview')));
  $output .= drupal_render_children($form);
  return $output;
}

/**
 * Recursive helper function for menu_overview_form().
 *
 * @param $tree
 *   The menu_tree retrieved by menu_tree_data.
 */
function _swift_framework_menu_overview_tree_form($tree) {
  $form = &drupal_static(__FUNCTION__, array('#tree' => TRUE));
  foreach ($tree as $data) {
    $title = '';
    $item = $data['link'];
    if ($item) {
      $mlid = 'mlid:' . $item['mlid'];
      $form[$mlid]['#item'] = $item;
      $form[$mlid]['#attributes'] = array('class' => array('menu-enabled'));
      $form[$mlid]['title']['#markup'] = l($item['link_title'], $item['link_path'], array());
      $form[$mlid]['weight'] = array(
        '#type' => 'weight',
        '#delta' => 50,
        '#default_value' => $item['weight'],
        '#title_display' => 'invisible',
        '#title' => t('Weight for @title', array('@title' => $item['link_title'])),
      );
      $form[$mlid]['mlid'] = array(
        '#type' => 'hidden',
        '#value' => $item['mlid'],
      );
      $form[$mlid]['plid'] = array(
        '#type' => 'hidden',
        '#default_value' => $item['plid'],
      );
      // Build a list of operations.
      $operations = array();
      $operations['edit'] = array(
        '#type' => 'link',
        '#title' => t('edit'),
        '#href' => 'admin/structure/swift-menu/item/' . $item['mlid'] . '/edit',
        '#options' => array('query' => array('destination' => $_GET['q']))
      );
      $operations['delete'] = array(
        '#type' => 'link',
        '#title' => t('delete'),
        '#href' => 'admin/structure/swift-menu/item/' . $item['mlid'] . '/delete',
        '#options' => array('query' => array('destination' => $_GET['q']))
      );
      $form[$mlid]['operations'] = $operations;
    }

    if ($data['below']) {
      _swift_framework_menu_overview_tree_form($data['below']);
    }
  }
  return $form;
}

/**
 * Submit handler for the menu overview form.
 *
 * This function takes great care in saving parent items first, then items
 * underneath them. Saving items in the incorrect order can break the menu tree.
 *
 * @see menu_overview_form()
 */
function swift_framework_menu_overview_form_submit($form, &$form_state) {
  // When dealing with saving menu items, the order in which these items are
  // saved is critical. If a changed child item is saved before its parent,
  // the child item could be saved with an invalid path past its immediate
  // parent. To prevent this, save items in the form in the same order they
  // are sent by $_POST, ensuring parents are saved first, then their children.
  // See http://drupal.org/node/181126#comment-632270
  $order = array_flip(array_keys($form_state['input'])); // Get the $_POST order.
  $form = array_merge($order, $form); // Update our original form with the new order.

  $updated_items = array();
  $fields = array('weight', 'plid', 'depth');
  foreach (element_children($form) as $mlid) {
    if (isset($form[$mlid]['#item'])) {
      $element = $form[$mlid];
      // Update any fields that have changed in this menu item.
      foreach ($fields as $field) {
        if (isset($element[$field]['#value']) && $element[$field]['#value'] != $element[$field]['#default_value']) {
          $element['#item'][$field] = $element[$field]['#value'];
          $updated_items[$mlid] = $element['#item'];
        }
      }
    }
  }

  // Save all our changed items to the database.
  foreach ($updated_items as $item) {
    _swift_framework_topic_menu_link_save($item);
  }
  drupal_set_message(t('Your configuration has been saved.'));
}

/**
 * Menu callback; Build the menu link editing form.
 */
function swift_framework_topic_menu_edit_item($form, &$form_state, $type, $item, $menu) {
  if ($type == 'add' || empty($item)) {
    // This is an add form, initialize the menu link.
    $item = array('link_title' => '', 'mlid' => 0, 'plid' => 0, 'menu_name' => $menu['menu_name'], 'weight' => 0, 'link_path' => '', 'depth' => 1);
  }
  $form['actions'] = array('#type' => 'actions');
  $form['link_title'] = array(
    '#type' => 'textfield',
    '#title' => t('Menu link title'),
    '#default_value' => $item['link_title'],
    '#description' => t('The text to be used for this link in the menu. <strong>Note: if the child page has a Tab Title set, it will override this title.</strong>'),
    '#maxlength' => 20,
  );
  foreach (array('link_path', 'mlid') as $key) {
    $form[$key] = array('#type' => 'value', '#value' => $item[$key]);
  }
  $form['original_item'] = array('#type' => 'value', '#value' => $item);

  $path = $item['link_path'];

  $form['link_path'] = array(
    '#type' => 'textfield',
    '#title' => t('Path'),
    '#maxlength' => 255,
    '#default_value' => $path,
    '#description' => t('The path for this menu link.'),
    '#required' => TRUE,
  );
  $form['actions']['delete'] = array(
    '#type' => 'submit',
    '#value' => t('Delete'),
    '#access' => $item['mlid'],
    '#submit' => array('swift_framework_menu_item_delete_submit'),
    '#weight' => 10,
  );

  // Generate a list of possible parents (not including this link or descendants).
  $options = swift_framework_topic_menu_parent_options($item);
  $default = $item['plid'];

  $form['plid'] = array(
    '#type' => 'select',
    '#title' => t('Parent link'),
    '#default_value' => $default,
    '#options' => array(t('- none -')) + $options,
    '#description' => t('The maximum depth for a link and all its children is fixed at !maxdepth. Some menu links may not be available as parents if selecting them would exceed this limit.', array('!maxdepth' => MENU_MAX_DEPTH)),
    '#attributes' => array('class' => array('menu-title-select')),
  );
  $form['weight'] = array(
    '#type' => 'weight',
    '#title' => t('Weight'),
    '#delta' => 50,
    '#default_value' => $item['weight'],
    '#description' => t('Optional. In the menu, the heavier links will sink and the lighter links will be positioned nearer the top.'),
  );
  $form['actions']['submit'] = array('#type' => 'submit', '#value' => t('Save'));

  $form['#original_item'] = $item;

  return $form;
}

/**
 * Validate form values for a menu link being added or edited.
 */
function swift_framework_topic_menu_edit_item_validate($form, &$form_state) {
  $original_item = $form['#original_item'];
  $item = &$form_state['values'];
  $normal_path = drupal_get_normal_path($item['link_path']);
  $input_path = $form_state['input']['link_path'];

  if ($item['link_path'] != $normal_path) {
    drupal_set_message(t('The menu system stores system paths only, but will use the URL alias for display. %link_path has been stored as %normal_path', array('%link_path' => $item['link_path'], '%normal_path' => $normal_path)));
    $item['link_path'] = $normal_path;
  }
  if (!url_is_external($item['link_path'])) {
    $parsed_link = parse_url($item['link_path']);
    if (isset($parsed_link['query'])) {
      $item['options']['query'] = drupal_get_query_array($parsed_link['query']);
    }
    else {
      // Use unset() rather than setting to empty string
      // to avoid redundant serialized data being stored.
      unset($item['options']['query']);
    }
    if (isset($parsed_link['path']) && $item['link_path'] != $parsed_link['path']) {
      $item['link_path'] = $parsed_link['path'];
    }
  }
  if (!trim($item['link_path']) || !drupal_valid_path($item['link_path'], TRUE)) {
    form_set_error('link_path', t("The path '@link_path' is either invalid or you do not have access to it.", array('@link_path' => $item['link_path'])));
  }

  // Validate language requirements coming from the topic
  // Validate topic child types
  $item_language = LANGUAGE_NONE;
  $nid = str_replace('node/', '', $normal_path);
  if (is_numeric($nid)) {
    $node = node_load($nid);
    $tab_title = _swift_framework_get_tab_title($node);
    if ($tab_title) {
      $form_state['values']['link_title'] = $tab_title;
    }

    $item_language = db_select('node', 'n')
      ->fields('n', array('language'))
      ->condition('nid', $nid)
      ->execute()
      ->fetchField();

    if (!swift_framework_type_is('topic_child', $node->type)) {
      form_set_error('link_path', t('The path %link_path refers to a node of type %type, which is not an allowed topic child page type.', array('%link_path' => $input_path, '%type' => $node->type)));
    }

    if (swift_framework_topic_child_translations($node)) {
      form_set_error('link_path', t('The path %link_path refers to a node that already has translations which are part of a topic menu.', array('%link_path' => $input_path, '%type' => $node->type)));
    }

  }

  if (empty($form_state['values']['link_title'])) {
    form_set_error('link_title', t('Link title is required, since the child page does not provide a tab title itself.'));
  }

  $topic_menu = swift_framework_topic_menu_load($original_item['menu_name']);
  if ($topic_menu['language'] !== LANGUAGE_NONE) {
    $topic_id = str_replace('topic_', '', $original_item['menu_name']);
    $topic_languages = swift_framework_get_topic_languages($topic_id);
    if (empty($topic_languages[$item_language])) {
      form_set_error('link_path', t('The path %link_path should refer to a child page with same language as the respective topic language.', array('%link_path' => $input_path)));
    }
  }

  // Child page can have only one parent topic
  $exists = db_select('swift_menu_links', 'sml')
    ->fields('sml', array('mlid'))
    ->condition('link_path', $normal_path)
    ->execute()
    ->fetchField();

  if ($exists && (!isset($item['mlid']) || $item['mlid'] != $exists)) {
    form_set_error('link_path', t('The path %link_path refers to a child page which already has a parent topic.', array('%link_path' => $input_path)));
  }
  
}

/**
 * Menu callback; Check access and present a confirm form for deleting a menu link.
 */
function swift_framework_topic_menu_item_delete_page($item) {
  return drupal_get_form('swift_framework_topic_menu_item_delete_form', $item);
}

/**
 * Build a confirm form for deletion of a single menu link.
 */
function swift_framework_topic_menu_item_delete_form($form, &$form_state, $item) {
  $form['#item'] = $item;
  return confirm_form($form, t('Are you sure you want to delete the menu link %item?', array('%item' => $item['link_title'])), 'admin/structure/swift-menu/manage/' . $item['menu_name']);
}

/**
 * Process menu delete form submissions.
 */
function swift_framework_topic_menu_item_delete_form_submit($form, &$form_state) {
  $item = $form['#item'];
  _swift_framework_menu_link_delete($item['mlid']);
  path_delete(array('source' => $item['link_path']));
  $t_args = array('%title' => $item['link_title']);
  drupal_set_message(t('The menu link %title has been deleted.', $t_args));
  watchdog('menu', 'Deleted menu link %title.', $t_args, WATCHDOG_NOTICE);
  $form_state['redirect'] = 'admin/structure/swift-menu/manage/' . $item['menu_name'];
}

/**
 * Process menu and menu item add/edit form submissions.
 */
function swift_framework_topic_menu_edit_item_submit($form, &$form_state) {
  $original_item = $form['#original_item'];
  $item = &$form_state['values'];

  // always save the path to the source node
  $nid = (int) str_replace('node/', '', $item['link_path']);
  if ($nid) {
    $tnid = swift_framework_get_tnid($nid);
    if ($tnid != $nid) {
      $item['link_path'] = 'node/'. $tnid;
      $item['link_title'] = _swift_framework_get_tab_title($tnid);
    }
  }

  $item['menu_name'] = $original_item['menu_name'];
  if (!_swift_framework_topic_menu_link_save($item)) {
    drupal_set_message(t('There was an error saving the swift menu link.'), 'error');
  }
  else {
    drupal_set_message(t('Your configuration has been saved.'));
  }
  $form_state['redirect'] = 'admin/structure/swift-menu/manage/' . $item['menu_name'];
}

/**
 * Menu callback - Topic menu tab on a node page
 */
function swift_framework_topic_menu_manage($node) {
  $topic_menu = (array) swift_framework_topic_menu_from_node($node);

  $add_link = l(
    t('Add exceptional child page'),
    'admin/structure/swift-menu/manage/'. $topic_menu['menu_name'] .'/add',
    array('query' => array('destination' =>'node/'. $node->nid .'/topic_menu'))
  );

  $form['#prefix'] = $add_link;
  $form += drupal_get_form('swift_framework_menu_overview_form', $topic_menu);

  return $form;
}

function swift_framework_topic_add_child($node) {
  $topic_menu = swift_framework_topic_menu_from_node($node);
  // get all child types
  $child_types = variable_get('swift_topic_child_types', array());

  $links = array();
  foreach ($child_types as $type) {
    $type = node_type_get_type($type);
    $links[] = array(
      'title' => $type->name,
      'description' => $type->description,
      'href' => 'node/add/'. str_replace('_', '-', $type->type),
      'localized_options' => array('query' => array('topic_id' => $topic_menu->nid)),
    );
  }
  return theme('node_add_list', array('content' => $links));
}
